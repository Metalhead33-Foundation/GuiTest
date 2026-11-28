#include "JsIO.hpp"
#include <ScriptHost.hpp>
#include <js/Object.h>
#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>

namespace JS {

namespace {

/// @brief The host the registered functions talk to; see `JsSystem.cpp`.
Sandbox::ScriptHost* gHost = nullptr;

/// @brief The `IO.File` wrapper class.
ClassCreator FileClass;

/// @brief Translates a JavaScript mode string into an `Elv::Io::Mode`.
Elv::Io::Mode parseMode(const std::string& mode)
{
	if (mode == "w" || mode == "write") return Elv::Io::Mode::WRITE;
	if (mode == "a" || mode == "append") return Elv::Io::Mode::APPEND;
	if (mode == "rw") return Elv::Io::Mode::READ_WRITE;
	if (mode == "ra") return Elv::Io::Mode::READ_APPEND;
	return Elv::Io::Mode::READ;
}

/// @brief Translates a JavaScript seek-origin string into an `Elv::Io::SeekOrigin`.
Elv::Io::SeekOrigin parseOrigin(const std::string& origin)
{
	if (origin == "cur" || origin == "current") return Elv::Io::SeekOrigin::CUR;
	if (origin == "end") return Elv::Io::SeekOrigin::END;
	return Elv::Io::SeekOrigin::SET;
}

/// @brief The device behind `this`, never null.
Elv::Io::Device* deviceFromThis(JS::CallArgs& args)
{
	auto holder = getNativePointer<Elv::Io::sDevice>(args.thisv().toObjectOrNull());
	if (!holder || !*holder) {
		throw std::runtime_error("IO.File: the device is not valid");
	}
	return holder->get();
}

} // namespace

namespace {

/// @brief `new IO.File(path, mode)`.
bool js_file_io_constructor(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		Sandbox::ScriptHost* host = gHost;
		if (host == nullptr) {
			throw std::runtime_error("IO.File: the filesystem is not available");
		}
		if (argc < 1) {
			throw std::runtime_error("IO.File: expected a path");
		}
		const std::string path = JsToNativeString(cx, args.get(0).toString());
		const std::string mode = argc >= 2 && args.get(1).isString()
			? JsToNativeString(cx, args.get(1).toString())
			: std::string("r");
		Elv::Io::sDevice device = host->io.openShared(path.c_str(), parseMode(mode));
		if (!device || !device->isValid()) {
			throw std::runtime_error("IO.File: could not open '" + path + "'");
		}
		JS::RootedObject object(cx,
			JS_NewObjectWithGivenProto(cx, &FileClass.protoClass, *FileClass.prototype));
		setNativePointer(object, new Elv::Io::sDevice(std::move(device)));
		args.rval().setObject(*object);
		return true;
	}, cx, argc, vp);
}

/// @brief Finalizer of `IO.File`: releases the shared device.
void js_file_io_finalizer(JS::GCContext* gcx, JSObject* obj)
{
	finalizeNative<Elv::Io::sDevice>(gcx, obj);
}

/// @brief `file.size`.
bool js_file_get_size(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), deviceFromThis(args)->size());
		return true;
	}, cx, argc, vp);
}

/// @brief `file.tell`.
bool js_file_get_tell(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), deviceFromThis(args)->tell());
		return true;
	}, cx, argc, vp);
}

/// @brief `file.eof`.
bool js_file_get_eof(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), deviceFromThis(args)->eof());
		return true;
	}, cx, argc, vp);
}

/// @brief `file.valid`.
bool js_file_get_valid(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), deviceFromThis(args)->isValid());
		return true;
	}, cx, argc, vp);
}

/// @brief `file.readString()`: everything from the current position.
bool js_file_read_string(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		Elv::Io::Device* device = deviceFromThis(args);
		std::string text;
		char buffer[4096];
		for (;;) {
			const std::size_t count = device->read(buffer, 1, sizeof(buffer));
			if (count == 0) break;
			text.append(buffer, count);
		}
		setReturnValue(cx, args.rval(), text);
		return true;
	}, cx, argc, vp);
}

/// @brief `file.read(count)`: at most @p count bytes from the current position.
bool js_file_read(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		Elv::Io::Device* device = deviceFromThis(args);
		const std::size_t wanted = argc >= 1
			? static_cast<std::size_t>(args.get(0).toNumber())
			: 0u;
		std::string text(wanted, '\0');
		if (wanted == 0) {
			args.rval().setString(JS_GetEmptyString(cx));
			return true;
		}
		const std::size_t count = device->read(text.data(), 1, wanted);
		text.resize(count);
		setReturnValue(cx, args.rval(), text);
		return true;
	}, cx, argc, vp);
}

} // namespace


namespace {

/// @brief `file.writeString(text)`: returns the number of bytes written.
bool js_file_write_string(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		Elv::Io::Device* device = deviceFromThis(args);
		const std::string text = argc >= 1 ? JsToNativeString(cx, args.get(0).toString())
										   : std::string();
		const std::size_t written = text.empty()
			? 0u
			: device->write(text.data(), 1, text.size());
		setReturnValue(cx, args.rval(), written);
		return true;
	}, cx, argc, vp);
}

/// @brief `file.seek(offset, origin)`: returns the resulting position.
bool js_file_seek(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		Elv::Io::Device* device = deviceFromThis(args);
		const long offset = argc >= 1 ? static_cast<long>(args.get(0).toNumber()) : 0L;
		const std::string origin = argc >= 2 && args.get(1).isString()
			? JsToNativeString(cx, args.get(1).toString())
			: std::string("set");
		if (device->seek(offset, parseOrigin(origin)) != 0) {
			throw std::runtime_error("IO.File.seek: the device rejected the offset");
		}
		setReturnValue(cx, args.rval(), device->tell());
		return true;
	}, cx, argc, vp);
}

/// @brief `file.flush()`.
bool js_file_flush(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), deviceFromThis(args)->flush());
		return true;
	}, cx, argc, vp);
}

/// @brief `IO.exists(path)`.
bool js_io_exists(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		Sandbox::ScriptHost* host = gHost;
		if (host == nullptr || argc < 1) {
			args.rval().setBoolean(false);
			return true;
		}
		const std::string path = JsToNativeString(cx, args.get(0).toString());
		args.rval().setBoolean(host->io.exists(path.c_str()));
		return true;
	}, cx, argc, vp);
}

/// @brief `IO.isFile(path)`.
bool js_io_is_file(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		Sandbox::ScriptHost* host = gHost;
		if (host == nullptr || argc < 1) {
			args.rval().setBoolean(false);
			return true;
		}
		const std::string path = JsToNativeString(cx, args.get(0).toString());
		args.rval().setBoolean(host->io.isFile(path.c_str()));
		return true;
	}, cx, argc, vp);
}

/// @brief `IO.isDirectory(path)`.
bool js_io_is_directory(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		Sandbox::ScriptHost* host = gHost;
		if (host == nullptr || argc < 1) {
			args.rval().setBoolean(false);
			return true;
		}
		const std::string path = JsToNativeString(cx, args.get(0).toString());
		args.rval().setBoolean(host->io.isDirectory(path.c_str()));
		return true;
	}, cx, argc, vp);
}

} // namespace

void CreateIOClasses(JSContext* cx, const JS::HandleObject& global, Sandbox::ScriptHost& host)
{
	gHost = &host;

	std::memset(&FileClass.protoClass, 0, sizeof(JSClass));
	std::memset(&FileClass.ops, 0, sizeof(JSClassOps));
	FileClass.name = "File";
	FileClass.nargs = 2;
	FileClass.constructor = js_file_io_constructor;
	FileClass.ops.construct = js_file_io_constructor;
	FileClass.ops.finalize = js_file_io_finalizer;
	FileClass.protoClass.cOps = &FileClass.ops;
	FileClass.protoClass.name = "File";
	// Foreground finalization: releasing the last reference closes a PhysFS file.
	FileClass.protoClass.flags = JSCLASS_HAS_RESERVED_SLOTS(1);
	FileClass.prototype = std::make_unique<JS::RootedObject>(cx,
		JS_NewObject(cx, &FileClass.protoClass));
	FileClass.ps.push_back(JS_PSG("size", js_file_get_size, JSPROP_ENUMERATE));
	FileClass.ps.push_back(JS_PSG("tell", js_file_get_tell, JSPROP_ENUMERATE));
	FileClass.ps.push_back(JS_PSG("eof", js_file_get_eof, JSPROP_ENUMERATE));
	FileClass.ps.push_back(JS_PSG("valid", js_file_get_valid, JSPROP_ENUMERATE));
	FileClass.ps.push_back(JS_PS_END);
	FileClass.static_ps.push_back(JS_PS_END);
	FileClass.fs.push_back(JS_FN("readString", js_file_read_string, 0, 0));
	FileClass.fs.push_back(JS_FN("read", js_file_read, 1, 0));
	FileClass.fs.push_back(JS_FN("writeString", js_file_write_string, 1, 0));
	FileClass.fs.push_back(JS_FN("seek", js_file_seek, 2, 0));
	FileClass.fs.push_back(JS_FN("flush", js_file_flush, 0, 0));
	FileClass.fs.push_back(JS_FS_END);
	FileClass.static_fs.push_back(JS_FS_END);
	FileClass.registerToJS(cx, global, *FileClass.prototype);

	JS_DefineFunction(cx, global, "exists", js_io_exists, 1, JSPROP_ENUMERATE);
	JS_DefineFunction(cx, global, "isFile", js_io_is_file, 1, JSPROP_ENUMERATE);
	JS_DefineFunction(cx, global, "isDirectory", js_io_is_directory, 1, JSPROP_ENUMERATE);
}

} // namespace JS

