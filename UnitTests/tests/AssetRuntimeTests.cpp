/**
 * @file AssetRuntimeTests.cpp
 * @brief Behavioural tests for Euphemy's asset runtime.
 *
 * These cover the invariants that the documentation promises rather than the happy path alone:
 * the residency state machine, budget accounting and eviction, admission control, cancellation
 * reporting and dependency-failure propagation. Every test drives the scheduler through
 * `pollCompletions()` exactly like an application would.
 */
#include <catch2/catch_test_macros.hpp>
#include <Euphemy/Asset/EuphAssetRuntime.hpp>
#include <Euphemy/Io/EuphMemoryDevice.hpp>
#include <Elvavena/Util/ElvThreadPool.hpp>
#include <chrono>
#include <cstddef>
#include <map>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace Euph::Asset;

namespace {

/** @brief Builds a catalog record with a unique path hint. */
AssetRecord makeRecord(AssetId id, const char* type, std::vector<AssetId> dependencies = {})
{
	AssetRecord record;
	record.id = id;
	record.path = "memory:" + std::to_string(id);
	record.storedSize = 32;
	record.type = type;
	record.dependencies = std::move(dependencies);
	return record;
}

/** @brief Loader that serves a fixed byte pattern from memory. */
Elv::Io::sDevice makeMemoryDevice(const AssetRecord& record)
{
	(void)record;
	std::vector<std::byte> bytes(32, std::byte { 0x5A });
	return std::make_shared<Euph::Io::MemoryDevice>(Elv::Io::Mode::READ, std::move(bytes));
}

/** @brief Decoder that stores the raw bytes as the payload. */
DecodedAsset decodeBytes(const AssetRecord& record, std::span<const std::byte> bytes)
{
	(void)record;
	DecodedAsset decoded;
	std::vector<std::byte> copy(bytes.begin(), bytes.end());
	decoded.payload = makeBytePayload(std::move(copy));
	decoded.cpuBytes = bytes.size();
	return decoded;
}

/** @brief Drives the scheduler until @p id reaches @p wanted, or fails after a second. */
void pumpUntil(StreamingScheduler& scheduler, AssetId id, ResidencyState wanted)
{
	for (int attempt = 0; attempt < 400; ++attempt) {
		scheduler.pollCompletions();
		if (scheduler.state(id) == wanted)
			return;
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}
	FAIL("asset " << id << " never reached state " << static_cast<int>(wanted));
}

}

TEST_CASE("ResourceRegistry enforces the residency state machine", "[asset][registry]")
{
	ResourceRegistry registry;
	const auto handle = registry.createHandle<ImageResource>(1);
	REQUIRE(registry.isAlive(handle));
	CHECK(registry.state(1) == ResidencyState::Known);

	// The happy path is legal, a shortcut is not.
	CHECK(registry.setState(1, ResidencyState::Requested));
	CHECK_FALSE(registry.setState(1, ResidencyState::Resident));
	CHECK(registry.state(1) == ResidencyState::Requested);
	CHECK(registry.failureReason(1) == FailureReason::InvalidTransition);

	CHECK(registry.setState(1, ResidencyState::LoadingIO));
	CHECK(registry.failureReason(1) == FailureReason::None); // a successful transition clears it
	CHECK(registry.setState(1, ResidencyState::Decoding));
	CHECK(registry.setState(1, ResidencyState::Resident));
	CHECK(registry.hasFlag(1, ResidencyFlag::CpuResident) == false);

	// Destroying bumps the generation, so the old handle is rejected.
	REQUIRE(registry.destroy(1));
	CHECK_FALSE(registry.isAlive(handle));
	CHECK(registry.state(1) == ResidencyState::Missing);
	CHECK(registry.failureReason(1) == FailureReason::AssetNotFound);

	const auto recreated = registry.createHandle<ImageResource>(1);
	CHECK(recreated.generation != handle.generation);
	CHECK(registry.isAlive(recreated));
}

TEST_CASE("ResourceRegistry keys handles by resource kind", "[asset][registry]")
{
	ResourceRegistry registry;
	const auto image = registry.createHandle<ImageResource>(5);
	REQUIRE(registry.isAlive(image));

	// Re-typing the same id invalidates handles of the previous kind.
	const auto audio = registry.createHandle<AudioResource>(5);
	CHECK(registry.isAlive(audio));
	CHECK_FALSE(registry.isAlive(image));
	CHECK_FALSE(registry.isAlive(ResourceHandle<ImageResource> { 5, audio.generation, audio.slot }));
}

TEST_CASE("ResourceRegistry refuses to destroy a strongly referenced entry", "[asset][registry]")
{
	ResourceRegistry registry;
	registry.createHandle<GenericBlobResource>(9);
	REQUIRE(registry.retain(9));
	CHECK_FALSE(registry.destroy(9));
	CHECK(registry.release(9));
	CHECK_FALSE(registry.release(9)); // underflow is rejected
	CHECK(registry.destroy(9));
}

TEST_CASE("Eviction requests are tracked and cleared with the state", "[asset][registry]")
{
	ResourceRegistry registry;
	ResidencyManager residency(registry, nullptr);
	registry.createHandle<GenericBlobResource>(11);
	residency.makeEvictable(11);
	// Known -> Evictable is illegal, so nothing was marked.
	CHECK_FALSE(registry.hasFlag(11, ResidencyFlag::EvictionRequested));

	registry.setState(11, ResidencyState::Requested);
	registry.setState(11, ResidencyState::LoadingIO);
	registry.setState(11, ResidencyState::Decoding);
	registry.setState(11, ResidencyState::Resident);
	REQUIRE(residency.makeEvictable(11));
	CHECK(registry.state(11) == ResidencyState::Evictable);
	CHECK(registry.hasFlag(11, ResidencyFlag::EvictionRequested));

	residency.clearEvictionRequest(11);
	CHECK_FALSE(registry.hasFlag(11, ResidencyFlag::EvictionRequested));

	REQUIRE(residency.makeEvictable(11));
	CHECK(registry.setState(11, ResidencyState::Resident));
	CHECK_FALSE(registry.hasFlag(11, ResidencyFlag::EvictionRequested));
}

TEST_CASE("ResidencyManager measures usage and evicts by policy", "[asset][residency]")
{
	ResourceRegistry registry;
	ResidencyManager residency(registry, nullptr);
	residency.setBudget(BudgetKind::CpuBytes, 100);

	AssetCatalog catalog;
	catalog.addRecord(makeRecord(1, "blob"));
	catalog.addRecord(makeRecord(2, "blob"));
	residency.setCatalog(&catalog);

	registry.createHandle<GenericBlobResource>(1);
	registry.createHandle<GenericBlobResource>(2);
	for (const AssetId id : { AssetId(1), AssetId(2) }) {
		registry.setState(id, ResidencyState::Requested);
		registry.setState(id, ResidencyState::LoadingIO);
		registry.setState(id, ResidencyState::Decoding);
		registry.setState(id, ResidencyState::Resident);
		registry.setCost(id, BudgetKind::CpuBytes, 80);
	}
	CHECK(residency.currentUsage(BudgetKind::CpuBytes) == 160);
	CHECK_FALSE(residency.withinBudgets());

	// Nothing is evictable yet, so the pass cannot satisfy the budget.
	const EvictionResult stalled = residency.evictUntilWithinBudgetDetailed();
	CHECK(stalled.evicted.empty());
	CHECK_FALSE(stalled.withinBudget);

	// Pinning one candidate leaves only the other for eviction.
	REQUIRE(residency.makeEvictable(1));
	REQUIRE(residency.makeEvictable(2));
	residency.pin(1);
	const EvictionResult result = residency.evictUntilWithinBudgetDetailed();
	REQUIRE(result.evicted.size() == 1);
	CHECK(result.evicted.front() == 2);
	CHECK(result.withinBudget);
	CHECK(registry.state(2) == ResidencyState::Known);
	CHECK(registry.state(1) == ResidencyState::Evictable);
	CHECK(registry.cpuCost(2) == 0);
}

TEST_CASE("StreamingScheduler loads dependencies before dependents", "[asset][scheduler]")
{
	AssetCatalog catalog;
	catalog.addRecord(makeRecord(1, "blob"));
	catalog.addRecord(makeRecord(2, "image", { 1 }));
	ResourceRegistry registry;
	Elv::Util::ThreadPool pool(2);
	StreamingScheduler scheduler(catalog, registry, pool, makeMemoryDevice, decodeBytes);

	REQUIRE(scheduler.request(2));
	pumpUntil(scheduler, 2, ResidencyState::Resident);

	CHECK(scheduler.state(1) == ResidencyState::Resident);
	CHECK(scheduler.state(2) == ResidencyState::Resident);
	CHECK(scheduler.pendingJobs() == 0);
	CHECK(scheduler.queuedRequests() == 0);
	CHECK(scheduler.waitingRequests() == 0);
	REQUIRE(registry.payload(2) != nullptr);
	REQUIRE(registry.payload(2)->data != nullptr);
	CHECK(registry.payload(2)->bytes == 32);
}

TEST_CASE("StreamingScheduler fails a dependent when its dependency fails", "[asset][scheduler]")
{
	AssetCatalog catalog;
	catalog.addRecord(makeRecord(1, "audio"));
	catalog.addRecord(makeRecord(2, "blob", { 1 }));
	ResourceRegistry registry;
	Elv::Util::ThreadPool pool(2);
	StreamingScheduler scheduler(catalog, registry, pool, makeMemoryDevice,
		[](const AssetRecord& record, std::span<const std::byte> bytes) {
			if (record.id == 1)
				throw std::runtime_error("synthetic decode failure");
			return decodeBytes(record, bytes);
		});

	REQUIRE(scheduler.request(2));
	pumpUntil(scheduler, 2, ResidencyState::Failed);

	CHECK(scheduler.state(1) == ResidencyState::Failed);
	CHECK(registry.failureReason(1) == FailureReason::DecodeError);
	CHECK(registry.failureReason(2) == FailureReason::DependencyFailed);
	CHECK(registry.failureMessage(2).find("1") != std::string::npos);
	CHECK(scheduler.waitingRequests() == 0);
}

TEST_CASE("StreamingScheduler reports cancellation as Canceled", "[asset][scheduler][cancel]")
{
	AssetCatalog catalog;
	catalog.addRecord(makeRecord(1, "blob"));
	ResourceRegistry registry;
	Elv::Util::ThreadPool pool(1);
	StreamingScheduler scheduler(catalog, registry, pool,
		[](const AssetRecord& record) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			return makeMemoryDevice(record);
		},
		decodeBytes);

	REQUIRE(scheduler.request(1));
	scheduler.pollCompletions(); // starts the deliberately slow load
	REQUIRE(scheduler.pendingJobs() == 1);
	REQUIRE(scheduler.cancel(1));
	CHECK(registry.failureReason(1) == FailureReason::Canceled);

	for (int attempt = 0; attempt < 400 && scheduler.pendingJobs() != 0; ++attempt) {
		scheduler.pollCompletions();
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}

	CHECK(scheduler.state(1) == ResidencyState::Failed);
	CHECK(registry.failureReason(1) == FailureReason::Canceled);
	// No payload may be installed for a cancelled request.
	CHECK((registry.payload(1) == nullptr || registry.payload(1)->data == nullptr));
	CHECK(scheduler.pendingJobs() == 0);
}

TEST_CASE("StreamingScheduler stalls while residency budgets are exhausted", "[asset][scheduler][budget]")
{
	AssetCatalog catalog;
	catalog.addRecord(makeRecord(1, "blob"));
	catalog.addRecord(makeRecord(2, "blob"));
	ResourceRegistry registry;
	ResidencyManager residency(registry, &catalog);
	residency.setBudget(BudgetKind::CpuBytes, 8);

	// Occupy the whole budget with an entry that cannot be evicted.
	registry.createHandle<GenericBlobResource>(1);
	registry.setState(1, ResidencyState::Requested);
	registry.setState(1, ResidencyState::LoadingIO);
	registry.setState(1, ResidencyState::Decoding);
	registry.setState(1, ResidencyState::Resident);
	registry.setCost(1, BudgetKind::CpuBytes, 16);

	Elv::Util::ThreadPool pool(2);
	StreamingSchedulerOptions options;
	options.residency = &residency;
	StreamingScheduler scheduler(catalog, registry, pool, makeMemoryDevice, decodeBytes, options);

	REQUIRE(scheduler.request(2));
	scheduler.pollCompletions();
	CHECK(scheduler.budgetBlocked());
	CHECK(scheduler.queuedRequests() == 1);
	CHECK(scheduler.state(2) == ResidencyState::Requested);

	// Releasing the pressure lets the queued request start on the next poll.
	registry.clearCosts(1);
	scheduler.pollCompletions();
	CHECK_FALSE(scheduler.budgetBlocked());
	pumpUntil(scheduler, 2, ResidencyState::Resident);
}

TEST_CASE("StreamingScheduler enforces the queued-request limit", "[asset][scheduler][budget]")
{
	AssetCatalog catalog;
	catalog.addRecord(makeRecord(1, "blob"));
	catalog.addRecord(makeRecord(2, "blob"));
	ResourceRegistry registry;
	Elv::Util::ThreadPool pool(2);
	StreamingSchedulerOptions options;
	options.maxQueuedRequests = 1;
	StreamingScheduler scheduler(catalog, registry, pool, makeMemoryDevice, decodeBytes, options);

	REQUIRE(scheduler.request(1));
	CHECK(scheduler.queuedRequests() == 1);
	CHECK_FALSE(scheduler.request(2));
	CHECK(registry.state(2) == ResidencyState::Failed);
	CHECK(registry.failureReason(2) == FailureReason::BudgetExceeded);
}
