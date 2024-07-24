#include "JsIO.hpp"
#include <JS/JsUtil.hpp>
#include <MhLib/Io/MhIoDevice.hpp>
#include <MhLib/IoSys/MhIoSystem.hpp>
#include <MhLib/Io/MhDataStream.hpp>
#include <js/Array.h>
#include <js/MapAndSet.h>
#include <vector>
#include <MhLib/Io/MhFile.hpp>
#include <MhLib/Io/PhysFSIO.hpp>
#include <MhLib/Io/MhStderr.hpp>
#include <MhLib/Io/MhStdin.hpp>
#include <MhLib/Io/MhStdout.hpp>
#include <MhLib/IoSys/MhFilesystem.hpp>
#include <MhLib/IoSys/PhysFSIoSystem.hpp>

namespace JS {
/*template <MH33::Util::Endian endianness> void js_write_object(MH33::Io::DataStream<endianness>& datastream, JSContext* cx, const JSObject& value);
template <MH33::Util::Endian endianness> static void js_write_value(MH33::Io::DataStream<endianness>& datastream, JSContext* cx, const JS::Value& value) {
	datastream << static_cast<uint8_t>(value.type());
	switch (value.type()) {
		case JS::ValueType::Double: datastream << value.toDouble(); break;
		case JS::ValueType::Int32: datastream << value.toInt32(); break;
		case JS::ValueType::Boolean: datastream << value.toBoolean(); break;
		case JS::ValueType::String: datastream << JsToNativeString(cx,value.toString()); break;
		case JS::ValueType::Object: js_write_object(datastream,cx, value.toObject());
	default: break;
	}
}
template <MH33::Util::Endian endianness> void js_write_object(MH33::Io::DataStream<endianness>& datastream, JSContext* cx, const JSObject& value) {
	bool isArray;
	bool isSet;
	bool isMap;
	JS::RootedObject rooted(cx, value);
	JS::IsArrayObject(cx, rooted, &isArray);
	JS::IsSetObject(cx, rooted, &isSet);
	JS::IsMapObject(cx, rooted, &isMap);
	if(isArray) {
		datastream << uint8_t(1);
		uint32_t jsArrLength;
		JS::GetArrayLength(cx,rooted,&jsArrLength);
		datastream << jsArrLength;
		for(uint32_t i = 0; i < jsArrLength; ++i) {
			JS::RootedValue rval(cx);
			JS_GetElement(cx,rooted,i,&rval);
			js_write_value(datastream,cx,rooted);
		}
	} else if(isSet) {
		datastream << uint8_t(2);
		// TODO
	}
}*/

// Half-float
template <MH33::Util::Endian endianness> static bool js_read_half_float(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		MH33::Io::sDevice rnd = JS::getSmartPointerFromObj<MH33::Io::Device>(args.thisv().toObjectOrNull(), 0);
		half_float::half half;
		MH33::Io::DataStream<endianness>(*rnd) >> half;
		args.rval().setNumber(half.operator float());
		return true;
	}, cx, argc, vp);
}
template <MH33::Util::Endian endianness> static bool js_read_half_float_array(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		MH33::Io::sDevice rnd = JS::getSmartPointerFromObj<MH33::Io::Device>(args.thisv().toObjectOrNull(), 0);
		std::vector<half_float::half> nativeArray;
		MH33::Io::DataStream<endianness>(*rnd) >> nativeArray;
		JS::RootedObject jsArray(cx, JS::NewArrayObject(cx,nativeArray.size()));
		for(size_t i = 0; i < nativeArray.size(); ++i)
		{
			JS_SetElement(cx,jsArray,i,nativeArray[i]);
		}
		args.rval().setObjectOrNull(jsArray);
		return true;
	}, cx, argc, vp);
}
template <MH33::Util::Endian endianness> static bool js_write_half_float(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		MH33::Io::sDevice rnd = JS::getSmartPointerFromObj<MH33::Io::Device>(args.thisv().toObjectOrNull(), 0);
		half_float::half half = half_float::half(args.get(0).toDouble());
		MH33::Io::DataStream<endianness>(*rnd) << half;
		args.rval().set(args.thisv());
		return true;
	}, cx, argc, vp);
}
template <MH33::Util::Endian endianness> static bool js_write_half_float_array(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		MH33::Io::sDevice rnd = JS::getSmartPointerFromObj<MH33::Io::Device>(args.thisv().toObjectOrNull(), 0);
		JS::RootedObject jsArray(cx, args.get(0).toObjectOrNull());
		bool isArray = true;
		JS::IsArrayObject(cx,jsArray,&isArray);
		if(!isArray) throw std::runtime_error("Must supply array as argument!");
		uint32_t jsArrLength;
		JS::GetArrayLength(cx,jsArray,&jsArrLength);
		std::vector<half_float::half> nativeArray(jsArrLength);
		for(size_t i = 0; i < nativeArray.size(); ++i)
		{
			JS::RootedValue rval(cx);
			JS_GetElement(cx,jsArray,i,&rval);
			nativeArray[i] = rval.toDouble();
		}
		MH33::Io::DataStream<endianness>(*rnd) << nativeArray;
		args.rval().setObjectOrNull(jsArray);
		return true;
	}, cx, argc, vp);
}
// Regular number
template <typename T, MH33::Util::Endian endianness> static bool js_read_number(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		(void)cx;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		MH33::Io::sDevice rnd = JS::getSmartPointerFromObj<MH33::Io::Device>(args.thisv().toObjectOrNull(), 0);
		T number;
		MH33::Io::DataStream<endianness>(*rnd) >> number;
		args.rval().setNumber(number);
		return true;
	}, cx, argc, vp);
}
template <typename T, MH33::Util::Endian endianness> static bool js_read_number_array(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		MH33::Io::sDevice rnd = JS::getSmartPointerFromObj<MH33::Io::Device>(args.thisv().toObjectOrNull(), 0);
		std::vector<T> nativeArray;
		MH33::Io::DataStream<endianness>(*rnd) >> nativeArray;
		JS::RootedObject jsArray(cx, JS::NewArrayObject(cx,nativeArray.size()));
		for(size_t i = 0; i < nativeArray.size(); ++i)
		{
			JS::RootedValue rootval(cx);
			rootval.setNumber(nativeArray[i]);
			JS_SetElement(cx,jsArray,i,rootval);
		}
		args.rval().setObjectOrNull(jsArray);
		return true;
	}, cx, argc, vp);
}
template <typename T, MH33::Util::Endian endianness> static bool js_write_number(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		MH33::Io::sDevice rnd = JS::getSmartPointerFromObj<MH33::Io::Device>(args.thisv().toObjectOrNull(), 0);
		T number = args.get(0).toNumber();
		MH33::Io::DataStream<endianness>(*rnd) << number;
		args.rval().set(args.thisv());
		return true;
	}, cx, argc, vp);
}
template <typename T, MH33::Util::Endian endianness> static bool js_write_number_array(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		MH33::Io::sDevice rnd = JS::getSmartPointerFromObj<MH33::Io::Device>(args.thisv().toObjectOrNull(), 0);
		JS::RootedObject jsArray(cx, args.get(0).toObjectOrNull());
		bool isArray = true;
		JS::IsArrayObject(cx,jsArray,&isArray);
		if(!isArray) throw std::runtime_error("Must supply array as argument!");
		uint32_t jsArrLength;
		JS::GetArrayLength(cx,jsArray,&jsArrLength);
		std::vector<T> nativeArray(jsArrLength);
		for(size_t i = 0; i < nativeArray.size(); ++i)
		{
			JS::RootedValue rval(cx);
			JS_GetElement(cx,jsArray,i,&rval);
			nativeArray[i] = rval.toNumber();
		}
		MH33::Io::DataStream<endianness>(*rnd) << nativeArray;
		args.rval().setObjectOrNull(jsArray);
		return true;
	}, cx, argc, vp);
}
// String
static bool js_read_all_as_string(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		MH33::Io::sDevice rnd = JS::getSmartPointerFromObj<MH33::Io::Device>(args.thisv().toObjectOrNull(), 0);
		std::string nativeString = rnd->readAllAsString();
		JS::RootedString jsString(cx,JS_NewStringCopyN(cx,nativeString.c_str(),nativeString.length()) );
		args.rval().setString(jsString);
		return true;
	}, cx, argc, vp);
}
static bool js_read_line(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		MH33::Io::sDevice rnd = JS::getSmartPointerFromObj<MH33::Io::Device>(args.thisv().toObjectOrNull(), 0);
		std::string nativeString = rnd->readLine();
		JS::RootedString jsString(cx,JS_NewStringCopyN(cx,nativeString.c_str(),nativeString.length()) );
		args.rval().setString(jsString);
		return true;
	}, cx, argc, vp);
}
template <MH33::Util::Endian endianness> static bool js_read_string(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		MH33::Io::sDevice rnd = JS::getSmartPointerFromObj<MH33::Io::Device>(args.thisv().toObjectOrNull(), 0);
		std::string nativeString;
		MH33::Io::DataStream<endianness>(*rnd) >> nativeString;
		JS::RootedString jsString(cx,JS_NewStringCopyN(cx,nativeString.c_str(),nativeString.length()) );
		args.rval().setString(jsString);
		return true;
	}, cx, argc, vp);
}
template <MH33::Util::Endian endianness> static bool js_read_string_array(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		MH33::Io::sDevice rnd = JS::getSmartPointerFromObj<MH33::Io::Device>(args.thisv().toObjectOrNull(), 0);
		std::vector<std::string> nativeArray;
		MH33::Io::DataStream<endianness>(*rnd) >> nativeArray;
		JS::RootedObject jsArray(cx, JS::NewArrayObject(cx,nativeArray.size()));
		for(size_t i = 0; i < nativeArray.size(); ++i)
		{
			JS::RootedString jsString(cx,JS_NewStringCopyN(cx,nativeArray[i].c_str(),nativeArray[i].length()) );
			JS_SetElement(cx,jsArray,i,jsString);
		}
		args.rval().setObjectOrNull(jsArray);
		return true;
	}, cx, argc, vp);
}
static bool js_write_string_raw(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		MH33::Io::sDevice rnd = JS::getSmartPointerFromObj<MH33::Io::Device>(args.thisv().toObjectOrNull(), 0);
		size_t written;
		std::string nativeString = JsToNativeString(cx,args.get(0).toString());
		if(argc >= 2) {
			if(args.get(1).toBoolean()) written = rnd->write(nativeString.c_str(),nativeString.size()+1);
			else written = rnd->write(nativeString.c_str(),nativeString.size());
		} else written = rnd->write(nativeString.c_str(),nativeString.size());
		args.rval().setInt32(written);
		return true;
	}, cx, argc, vp);
}
template <MH33::Util::Endian endianness> static bool js_write_string(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		MH33::Io::sDevice rnd = JS::getSmartPointerFromObj<MH33::Io::Device>(args.thisv().toObjectOrNull(), 0);
		std::string nativeString = JsToNativeString(cx,args.get(0).toString());
		MH33::Io::DataStream<endianness>(*rnd) << nativeString;
		args.rval().set(args.thisv());
		return true;
	}, cx, argc, vp);
}
template <MH33::Util::Endian endianness> static bool js_write_string_array(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		MH33::Io::sDevice rnd = JS::getSmartPointerFromObj<MH33::Io::Device>(args.thisv().toObjectOrNull(), 0);
		JS::RootedObject jsArray(cx, args.get(0).toObjectOrNull());
		bool isArray = true;
		JS::IsArrayObject(cx,jsArray,&isArray);
		if(!isArray) throw std::runtime_error("Must supply array as argument!");
		uint32_t jsArrLength;
		JS::GetArrayLength(cx,jsArray,&jsArrLength);
		std::vector<std::string> nativeArray(jsArrLength);
		for(size_t i = 0; i < nativeArray.size(); ++i)
		{
			JS::RootedValue rval(cx);
			JS_GetElement(cx,jsArray,i,&rval);
			nativeArray[i] = JsToNativeString(cx,rval.toString());
		}
		MH33::Io::DataStream<endianness>(*rnd) << nativeArray;
		args.rval().setObjectOrNull(jsArray);
		return true;
	}, cx, argc, vp);
}
static bool js_file_seek(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		if(argc < 2) {
			throw std::runtime_error("File seek requires at least two arguments!");
		}
		MH33::Io::sDevice rnd = JS::getSmartPointerFromObj<MH33::Io::Device>(args.thisv().toObjectOrNull(), 0);
		switch (args.get(0).toInt32()) {
			case int32_t(MH33::Io::SeekOrigin::CUR): args.rval().setInt32( rnd->seek(MH33::Io::SeekOrigin::CUR, args.get(1).toInt32() ) ); break;
			case int32_t(MH33::Io::SeekOrigin::END): args.rval().setInt32( rnd->seek(MH33::Io::SeekOrigin::END, args.get(1).toInt32() ) ); break;
			case int32_t(MH33::Io::SeekOrigin::SET): args.rval().setInt32( rnd->seek(MH33::Io::SeekOrigin::SET, args.get(1).toInt32() ) ); break;
			default: throw std::runtime_error("Invalid seek origin!");
		}
		args.rval().setInt32( uint8_t(rnd->getMode()) );
		return true;
	}, cx, argc, vp);
}
static bool js_file_get_mode(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		MH33::Io::sDevice rnd = JS::getSmartPointerFromObj<MH33::Io::Device>(args.thisv().toObjectOrNull(), 0);
		args.rval().setInt32( uint8_t(rnd->getMode()) );
		return true;
	}, cx, argc, vp);
}
static bool js_file_flush(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		MH33::Io::sDevice rnd = JS::getSmartPointerFromObj<MH33::Io::Device>(args.thisv().toObjectOrNull(), 0);
		args.rval().setBoolean(rnd->flush());
		return true;
	}, cx, argc, vp);
}
static bool js_file_tell(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		MH33::Io::sDevice rnd = JS::getSmartPointerFromObj<MH33::Io::Device>(args.thisv().toObjectOrNull(), 0);
		args.rval().setInt32(rnd->tell());
		return true;
	}, cx, argc, vp);
}
static bool js_file_size(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		MH33::Io::sDevice rnd = JS::getSmartPointerFromObj<MH33::Io::Device>(args.thisv().toObjectOrNull(), 0);
		args.rval().setInt32(rnd->size());
		return true;
	}, cx, argc, vp);
}
static ClassCreator MhFSIoCreator;
static bool js_file_io_constructor(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		if(argc < 2) throw std::runtime_error("Needs at least 2 or more arguments!");
		int32_t mode = args.get(1).toInt32();
		std::string path = JsToNativeString(cx,args.get(0).toString());
		JS::RootedObject obj(cx, JS_NewObjectWithGivenProto(cx,&MhFSIoCreator.protoClass,*MhFSIoCreator.prototype));
		std::shared_ptr<MH33::Io::File> ptr(nullptr);
		switch (mode) {
		case int32_t( MH33::Io::Mode::APPEND ): ptr = std::make_shared<MH33::Io::File>( path, MH33::Io::Mode::APPEND ); break;
		case int32_t( MH33::Io::Mode::READ ): ptr = std::make_shared<MH33::Io::File>( path, MH33::Io::Mode::READ ); break;
		case int32_t( MH33::Io::Mode::READ_APPEND ): ptr = std::make_shared<MH33::Io::File>(path, MH33::Io::Mode::READ_APPEND); break;
		case int32_t( MH33::Io::Mode::READ_WRITE ): ptr = std::make_shared<MH33::Io::File>( path, MH33::Io::Mode::READ_WRITE); break;
		case int32_t( MH33::Io::Mode::WRITE ): ptr = std::make_shared<MH33::Io::File>( path, MH33::Io::Mode::WRITE); break;
		default: throw std::runtime_error("Invalid file mode for opening file!");
		}
		storeSmartPointerToObj(obj,0,ptr);
		args.rval().setObject(*obj);
		return true;
	}, cx, argc, vp);
}
static ClassCreator MhPhysFSCreator;
static bool js_physfs_io_constructor(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		if(argc < 2) throw std::runtime_error("Needs at least 2 or more arguments!");
		int32_t mode = args.get(1).toInt32();
		std::string path = JsToNativeString(cx,args.get(0).toString());
		JS::RootedObject obj(cx, JS_NewObjectWithGivenProto(cx,&MhPhysFSCreator.protoClass,*MhPhysFSCreator.prototype));
		std::shared_ptr<PhysFS::IO> ptr(nullptr);
		switch (mode) {
		case int32_t( MH33::Io::Mode::APPEND ): ptr = std::make_shared<PhysFS::IO>(path, MH33::Io::Mode::APPEND ); break;
		case int32_t( MH33::Io::Mode::READ ): ptr = std::make_shared<PhysFS::IO>( path, MH33::Io::Mode::READ ); break;
		case int32_t( MH33::Io::Mode::READ_APPEND ): ptr = std::make_shared<PhysFS::IO>( path, MH33::Io::Mode::READ_APPEND ); break;
		case int32_t( MH33::Io::Mode::READ_WRITE ): ptr = std::make_shared<PhysFS::IO>( path, MH33::Io::Mode::READ_WRITE ); break;
		case int32_t( MH33::Io::Mode::WRITE ): ptr = std::make_shared<PhysFS::IO>( path, MH33::Io::Mode::WRITE ); break;
		default: throw std::runtime_error("Invalid file mode for opening file!");
		}
		storeSmartPointerToObj(obj,0,ptr);
		args.rval().setObject(*obj);
		return true;
	}, cx, argc, vp);
}
static ClassCreator MhStdinCreator;
static bool js_stdin_io_constructor(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		JS::RootedObject obj(cx, JS_NewObjectWithGivenProto(cx,&MhStdinCreator.protoClass,*MhStdinCreator.prototype));
		storeSmartPointerToObj( obj, 0, std::make_shared<MH33::Io::Stdin>() );
		args.rval().setObject(*obj);
		return true;
	}, cx, argc, vp);
}
static ClassCreator MhStdoutCreator;
static bool js_stdout_io_constructor(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		JS::RootedObject obj(cx, JS_NewObjectWithGivenProto(cx,&MhStdoutCreator.protoClass,*MhStdoutCreator.prototype));
		storeSmartPointerToObj( obj, 0, std::make_shared<MH33::Io::Stdout>() );
		args.rval().setObject(*obj);
		return true;
	}, cx, argc, vp);
}
static ClassCreator MhStderrCreator;
static bool js_stderr_io_constructor(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		JS::RootedObject obj(cx, JS_NewObjectWithGivenProto(cx,&MhStderrCreator.protoClass,*MhStderrCreator.prototype));
		storeSmartPointerToObj( obj, 0, std::make_shared<MH33::Io::Stderr>() );
		args.rval().setObject(*obj);
		return true;
	}, cx, argc, vp);
}
static ClassCreator MhFilesystemCreator;
static ClassCreator MhPhysfsSystemCreator;
static bool hasIoBeenInitialized = false;

void CreateIOClasses(JSContext *cx, const JS::HandleObject &global)
{
	if(!hasIoBeenInitialized) {
		hasIoBeenInitialized = true;
		// Regular FileIO
		std::memset(&MhFSIoCreator.protoClass, 0, sizeof(JSClass));
		std::memset(&MhFSIoCreator.ops, 0, sizeof(JSClassOps));
		MhFSIoCreator.constructor = js_file_io_constructor;
		MhFSIoCreator.name = "File";
		MhFSIoCreator.nargs = 2;
		MhFSIoCreator.ops.finalize = js_finalizer_shared;
		MhFSIoCreator.ops.construct = js_file_io_constructor;
		MhFSIoCreator.protoClass.cOps = &MhFSIoCreator.ops;
		MhFSIoCreator.protoClass.name = "File";
		MhFSIoCreator.protoClass.flags = JSCLASS_HAS_RESERVED_SLOTS(1) | JSCLASS_BACKGROUND_FINALIZE;
		MhFSIoCreator.prototype = std::make_unique<JS::RootedObject>(cx, JS_NewObject(cx,&MhFSIoCreator.protoClass));
		MhFSIoCreator.ps.push_back(JS_PSG("size",js_file_size,JSPROP_ENUMERATE));
		MhFSIoCreator.ps.push_back(JS_PS_END);
		MhFSIoCreator.static_ps.push_back(JS_PS_END);
		MhFSIoCreator.fs.push_back(JS_FN("tell",js_file_tell,0,0));
		MhFSIoCreator.fs.push_back(JS_FN("flush",js_file_flush,0,0));
		MhFSIoCreator.fs.push_back(JS_FN("getMode",js_file_get_mode,0,0));
		//MhFSIoCreator.fs.push_back(JS_FN("size",js_file_size,0,0));
		MhFSIoCreator.fs.push_back(JS_FN("seek",js_file_seek,2,0));
		// Read stuff
		// 16-but half-float
		MhFSIoCreator.fs.push_back(JS_FN("readFloat16BE",js_read_half_float<MH33::Util::Endian::Big>,0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readFloat16ArrayBE",js_read_half_float_array<MH33::Util::Endian::Big>,0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readFloat16LE",js_read_half_float<MH33::Util::Endian::Little>,0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readFloat16ArrayLE",js_read_half_float_array<MH33::Util::Endian::Little>,0,0));
		// 32-bit single-precision float
		MhFSIoCreator.fs.push_back(JS_FN("readFloat32BE",(js_read_number<float, MH33::Util::Endian::Big>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readFloat32ArrayBE",(js_read_number_array<float, MH33::Util::Endian::Big>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readFloat32LE",(js_read_number<float, MH33::Util::Endian::Little>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readFloat32ArrayLE",(js_read_number_array<float, MH33::Util::Endian::Little>),0,0));
		// 64-bit double-precision float
		MhFSIoCreator.fs.push_back(JS_FN("readFloat64BE",(js_read_number<double, MH33::Util::Endian::Big>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readFloat64ArrayBE",(js_read_number_array<double, MH33::Util::Endian::Big>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readFloat64LE",(js_read_number<double, MH33::Util::Endian::Little>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readFloat64ArrayLE",(js_read_number_array<double, MH33::Util::Endian::Little>),0,0));
		// 8-but integers
		MhFSIoCreator.fs.push_back(JS_FN("readInt8",(js_read_number<int8_t, MH33::Util::Endian::Native>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readInt8ArrayBE",(js_read_number_array<int8_t, MH33::Util::Endian::Big>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readInt8ArrayLE",(js_read_number_array<int8_t, MH33::Util::Endian::Little>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readUint8",(js_read_number<uint8_t, MH33::Util::Endian::Native>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readUint8ArrayBE",(js_read_number_array<uint8_t, MH33::Util::Endian::Big>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readUint8ArrayLE",(js_read_number_array<uint8_t, MH33::Util::Endian::Little>),0,0));
		// 16-but integers
		MhFSIoCreator.fs.push_back(JS_FN("readInt16BE",(js_read_number<int16_t, MH33::Util::Endian::Big>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readInt16ArrayBE",(js_read_number_array<int16_t, MH33::Util::Endian::Big>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readInt16LE",(js_read_number<int16_t, MH33::Util::Endian::Little>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readInt16ArrayLE",(js_read_number_array<int16_t, MH33::Util::Endian::Little>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readUint16BE",(js_read_number<uint16_t, MH33::Util::Endian::Big>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readUint16ArrayBE",(js_read_number_array<uint16_t, MH33::Util::Endian::Big>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readUint16LE",(js_read_number<uint16_t, MH33::Util::Endian::Little>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readUint16ArrayLE",(js_read_number_array<uint16_t, MH33::Util::Endian::Little>),0,0));
		// 32-but integers
		MhFSIoCreator.fs.push_back(JS_FN("readInt32BE",(js_read_number<int32_t, MH33::Util::Endian::Big>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readInt32ArrayBE",(js_read_number_array<int32_t, MH33::Util::Endian::Big>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readInt32LE",(js_read_number<int32_t, MH33::Util::Endian::Little>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readInt32ArrayLE",(js_read_number_array<int32_t, MH33::Util::Endian::Little>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readUint32BE",(js_read_number<uint32_t, MH33::Util::Endian::Big>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readUint32ArrayBE",(js_read_number_array<uint32_t, MH33::Util::Endian::Big>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readUint32LE",(js_read_number<uint32_t, MH33::Util::Endian::Little>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readUint32ArrayLE",(js_read_number_array<uint32_t, MH33::Util::Endian::Little>),0,0));
		// 64-but integers
		MhFSIoCreator.fs.push_back(JS_FN("readInt64BE",(js_read_number<int64_t, MH33::Util::Endian::Big>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readInt64ArrayBE",(js_read_number_array<int64_t, MH33::Util::Endian::Big>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readInt64LE",(js_read_number<int64_t, MH33::Util::Endian::Little>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readInt64ArrayLE",(js_read_number_array<int64_t, MH33::Util::Endian::Little>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readUint64BE",(js_read_number<uint64_t, MH33::Util::Endian::Big>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readUint64ArrayBE",(js_read_number_array<uint64_t, MH33::Util::Endian::Big>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readUint64LE",(js_read_number<uint64_t, MH33::Util::Endian::Little>),0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readUint64ArrayLE",(js_read_number_array<uint64_t, MH33::Util::Endian::Little>),0,0));
		// Strings
		MhFSIoCreator.fs.push_back(JS_FN("readLine",js_read_line,0,0));
		MhFSIoCreator.fs.push_back(JS_FN("readEntireFileAsString",js_read_all_as_string,0,0));
		// Writing stuff
		// 16-but half-float
		MhFSIoCreator.fs.push_back(JS_FN("writeFloat16BE",js_write_half_float<MH33::Util::Endian::Big>,1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeFloat16ArrayBE",js_write_half_float_array<MH33::Util::Endian::Big>,1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeFloat16LE",js_write_half_float<MH33::Util::Endian::Little>,1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeFloat16ArrayLE",js_write_half_float_array<MH33::Util::Endian::Little>,1,0));
		// 32-bit single-precision float
		MhFSIoCreator.fs.push_back(JS_FN("writeFloat32BE",(js_write_number<float, MH33::Util::Endian::Big>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeFloat32ArrayBE",(js_write_number_array<float, MH33::Util::Endian::Big>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeFloat32LE",(js_write_number<float, MH33::Util::Endian::Little>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeFloat32ArrayLE",(js_write_number_array<float, MH33::Util::Endian::Little>),1,0));
		// 64-bit double-precision float
		MhFSIoCreator.fs.push_back(JS_FN("writeFloat64BE",(js_write_number<double, MH33::Util::Endian::Big>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeFloat64ArrayBE",(js_write_number_array<double, MH33::Util::Endian::Big>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeFloat64LE",(js_write_number<double, MH33::Util::Endian::Little>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeFloat64ArrayLE",(js_write_number_array<double, MH33::Util::Endian::Little>),1,0));
		// 8-but integers
		MhFSIoCreator.fs.push_back(JS_FN("writeInt8",(js_write_number<int8_t, MH33::Util::Endian::Native>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeInt8ArrayBE",(js_write_number_array<int8_t, MH33::Util::Endian::Big>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeInt8ArrayLE",(js_write_number_array<int8_t, MH33::Util::Endian::Little>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeUint8",(js_write_number<uint8_t, MH33::Util::Endian::Native>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeUint8ArrayBE",(js_write_number_array<uint8_t, MH33::Util::Endian::Big>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeUint8ArrayLE",(js_write_number_array<uint8_t, MH33::Util::Endian::Little>),1,0));
		// 16-but integers
		MhFSIoCreator.fs.push_back(JS_FN("writeInt16BE",(js_write_number<int16_t, MH33::Util::Endian::Big>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeInt16ArrayBE",(js_write_number_array<int16_t, MH33::Util::Endian::Big>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeInt16LE",(js_write_number<int16_t, MH33::Util::Endian::Little>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeInt16ArrayLE",(js_write_number_array<int16_t, MH33::Util::Endian::Little>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeUint16BE",(js_write_number<uint16_t, MH33::Util::Endian::Big>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeUint16ArrayBE",(js_write_number_array<uint16_t, MH33::Util::Endian::Big>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeUint16LE",(js_write_number<uint16_t, MH33::Util::Endian::Little>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeUint16ArrayLE",(js_write_number_array<uint16_t, MH33::Util::Endian::Little>),1,0));
		// 32-but integers
		MhFSIoCreator.fs.push_back(JS_FN("writeInt32BE",(js_write_number<int32_t, MH33::Util::Endian::Big>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeInt32ArrayBE",(js_write_number_array<int32_t, MH33::Util::Endian::Big>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeInt32LE",(js_write_number<int32_t, MH33::Util::Endian::Little>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeInt32ArrayLE",(js_write_number_array<int32_t, MH33::Util::Endian::Little>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeUint32BE",(js_write_number<uint32_t, MH33::Util::Endian::Big>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeUint32ArrayBE",(js_write_number_array<uint32_t, MH33::Util::Endian::Big>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeUint32LE",(js_write_number<uint32_t, MH33::Util::Endian::Little>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeUint32ArrayLE",(js_write_number_array<uint32_t, MH33::Util::Endian::Little>),1,0));
		// 64-but integers
		MhFSIoCreator.fs.push_back(JS_FN("writeInt64BE",(js_write_number<int64_t, MH33::Util::Endian::Big>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeInt64ArrayBE",(js_write_number_array<int64_t, MH33::Util::Endian::Big>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeInt64LE",(js_write_number<int64_t, MH33::Util::Endian::Little>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeInt64ArrayLE",(js_write_number_array<int64_t, MH33::Util::Endian::Little>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeUint64BE",(js_write_number<uint64_t, MH33::Util::Endian::Big>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeUint64ArrayBE",(js_write_number_array<uint64_t, MH33::Util::Endian::Big>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeUint64LE",(js_write_number<uint64_t, MH33::Util::Endian::Little>),1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeUint64ArrayLE",(js_write_number_array<uint64_t, MH33::Util::Endian::Little>),1,0));
		// Strings
		MhFSIoCreator.fs.push_back(JS_FN("writeString",js_write_string_raw,1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeStringBE",js_write_string<MH33::Util::Endian::Big>,1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeStringArrayBE",js_write_string_array<MH33::Util::Endian::Big>,1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeStringLE",js_write_string<MH33::Util::Endian::Little>,1,0));
		MhFSIoCreator.fs.push_back(JS_FN("writeStringArrayLE",js_write_string_array<MH33::Util::Endian::Little>,1,0));
		MhFSIoCreator.fs.push_back(JS_FS_END);
		MhFSIoCreator.static_fs.push_back(JS_FS_END);
		{
			auto toReturn = MhFSIoCreator.registerToJS(cx, global, *MhFSIoCreator.prototype);
			auto reg = JS::RootedValue(cx,JS::ObjectOrNullValue(toReturn) );
			auto rootedReg = JS::RootedObject(cx,toReturn);
			JS_SetProperty(cx, global, MhFSIoCreator.name.c_str(), reg );
		}
		// PhysFS
		std::memset(&MhPhysFSCreator.protoClass, 0, sizeof(JSClass));
		std::memset(&MhPhysFSCreator.ops, 0, sizeof(JSClassOps));
		MhPhysFSCreator.constructor = js_physfs_io_constructor;
		MhPhysFSCreator.name = "PhysFsFile";
		MhPhysFSCreator.nargs = 2;
		MhPhysFSCreator.ops.finalize = js_finalizer_shared;
		MhPhysFSCreator.ops.construct = js_physfs_io_constructor;
		MhPhysFSCreator.protoClass.cOps = &MhPhysFSCreator.ops;
		MhPhysFSCreator.protoClass.name = "PhysFsFile";
		MhPhysFSCreator.protoClass.flags = JSCLASS_HAS_RESERVED_SLOTS(1) | JSCLASS_BACKGROUND_FINALIZE;
		MhPhysFSCreator.prototype = std::make_unique<JS::RootedObject>(cx, JS_NewObject(cx,&MhPhysFSCreator.protoClass));
		MhPhysFSCreator.fs = MhFSIoCreator.fs;
		MhPhysFSCreator.ps = MhFSIoCreator.ps;
		MhPhysFSCreator.static_fs = MhFSIoCreator.static_fs;
		MhPhysFSCreator.static_ps = MhFSIoCreator.static_ps;
		{
			auto toReturn = MhPhysFSCreator.registerToJS(cx, global, *MhPhysFSCreator.prototype);
			auto reg = JS::RootedValue(cx,JS::ObjectOrNullValue(toReturn) );
			auto rootedReg = JS::RootedObject(cx,toReturn);
			JS_SetProperty(cx, global, MhPhysFSCreator.name.c_str(), reg );
		}
		// StdIN
		std::memset(&MhStdinCreator.protoClass, 0, sizeof(JSClass));
		std::memset(&MhStdinCreator.ops, 0, sizeof(JSClassOps));
		MhStdinCreator.constructor = js_stdin_io_constructor;
		MhStdinCreator.name = "StdIn";
		MhStdinCreator.nargs = 0;
		MhStdinCreator.ops.finalize = js_finalizer_shared;
		MhStdinCreator.ops.construct = js_stdin_io_constructor;
		MhStdinCreator.protoClass.cOps = &MhStdinCreator.ops;
		MhStdinCreator.protoClass.name = "StdIn";
		MhStdinCreator.protoClass.flags = JSCLASS_HAS_RESERVED_SLOTS(1) | JSCLASS_BACKGROUND_FINALIZE;
		MhStdinCreator.prototype = std::make_unique<JS::RootedObject>(cx, JS_NewObject(cx,&MhStdinCreator.protoClass));
		MhStdinCreator.fs = MhFSIoCreator.fs;
		MhStdinCreator.ps = MhFSIoCreator.ps;
		MhStdinCreator.static_fs = MhFSIoCreator.static_fs;
		MhStdinCreator.static_ps = MhFSIoCreator.static_ps;
		{
			auto toReturn = MhStdinCreator.registerToJS(cx, global, *MhStdinCreator.prototype);
			auto reg = JS::RootedValue(cx,JS::ObjectOrNullValue(toReturn) );
			auto rootedReg = JS::RootedObject(cx,toReturn);
			JS_SetProperty(cx, global, MhStdinCreator.name.c_str(), reg );
		}
		// StdOUT
		std::memset(&MhStdoutCreator.protoClass, 0, sizeof(JSClass));
		std::memset(&MhStdoutCreator.ops, 0, sizeof(JSClassOps));
		MhStdoutCreator.constructor = js_stdout_io_constructor;
		MhStdoutCreator.name = "StdOut";
		MhStdoutCreator.nargs = 0;
		MhStdoutCreator.ops.finalize = js_finalizer_shared;
		MhStdoutCreator.ops.construct = js_stdout_io_constructor;
		MhStdoutCreator.protoClass.cOps = &MhStdoutCreator.ops;
		MhStdoutCreator.protoClass.name = "StdOut";
		MhStdoutCreator.protoClass.flags = JSCLASS_HAS_RESERVED_SLOTS(1) | JSCLASS_BACKGROUND_FINALIZE;
		MhStdoutCreator.prototype = std::make_unique<JS::RootedObject>(cx, JS_NewObject(cx,&MhStdoutCreator.protoClass));
		MhStdoutCreator.fs = MhFSIoCreator.fs;
		MhStdoutCreator.ps = MhFSIoCreator.ps;
		MhStdoutCreator.static_fs = MhFSIoCreator.static_fs;
		MhStdoutCreator.static_ps = MhFSIoCreator.static_ps;
		{
			auto toReturn = MhStdoutCreator.registerToJS(cx, global, *MhStdoutCreator.prototype);
			auto reg = JS::RootedValue(cx,JS::ObjectOrNullValue(toReturn) );
			auto rootedReg = JS::RootedObject(cx,toReturn);
			JS_SetProperty(cx, global, MhStdoutCreator.name.c_str(), reg );
		}
		// StdErr
		std::memset(&MhStderrCreator.protoClass, 0, sizeof(JSClass));
		std::memset(&MhStderrCreator.ops, 0, sizeof(JSClassOps));
		MhStderrCreator.constructor = js_stderr_io_constructor;
		MhStderrCreator.name = "StdErr";
		MhStderrCreator.nargs = 0;
		MhStderrCreator.ops.finalize = js_finalizer_shared;
		MhStderrCreator.ops.construct = js_stderr_io_constructor;
		MhStderrCreator.protoClass.cOps = &MhStderrCreator.ops;
		MhStderrCreator.protoClass.name = "StdErr";
		MhStderrCreator.protoClass.flags = JSCLASS_HAS_RESERVED_SLOTS(1) | JSCLASS_BACKGROUND_FINALIZE;
		MhStderrCreator.prototype = std::make_unique<JS::RootedObject>(cx, JS_NewObject(cx,&MhStderrCreator.protoClass));
		MhStderrCreator.fs = MhFSIoCreator.fs;
		MhStderrCreator.ps = MhFSIoCreator.ps;
		MhStderrCreator.static_fs = MhFSIoCreator.static_fs;
		MhStderrCreator.static_ps = MhFSIoCreator.static_ps;
		{
			auto toReturn = MhStderrCreator.registerToJS(cx, global, *MhStderrCreator.prototype);
			auto reg = JS::RootedValue(cx,JS::ObjectOrNullValue(toReturn) );
			auto rootedReg = JS::RootedObject(cx,toReturn);
			JS_SetProperty(cx, global, MhStderrCreator.name.c_str(), reg );
		}
	}
}

}
