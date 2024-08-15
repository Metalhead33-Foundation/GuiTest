#include "JsGuiWrappers.hpp"
#include <GUI/MhTexturedButton.hpp>
#include <js/Array.h>
#include <js/Object.h>
#include <js/Value.h>
namespace JS {

ClassCreator MhTexturedButtonClass;
static bool hasIoBeenInitialized = false;

double dumbWorkaround[] = { 0, 0, 1, 0.2, 0, 0.2, 1, 0.4, 0, 0, 1, 0.2, 0, 0.4, 1, 0.6, 0, 0, 1, 0.2, 0, 0.6, 1, 0.8, 0, 0, 0, 0, 0, 0.8, 1, 1 };
std::unordered_map<void*,JS::PersistentRootedObject> persistentMap;

// void* textureAtlasId, const std::initializer_list<const TextureCoordDuo>& coordDuos
static bool js_gui_textured_button_constructor(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto arg0 = args.get(0);
		if(!arg0.isObject()) throw std::runtime_error("Excepted object as first argument!");
		RootedObject rootedObject0(cx,arg0.toObjectOrNull());
		auto arg1 = args.get(1);
		if(!arg1.isObject()) throw std::runtime_error("Excepted array object as second argument!");
		RootedObject rootedObject1(cx,arg1.toObjectOrNull());
		RootedValue propval(cx);
		JS_GetProperty(cx,rootedObject0,"tex",&propval);
		void* texref = propval.toPrivate();
		bool isArray;
		JS::IsArray(cx,rootedObject1,&isArray);
		if(!isArray) throw std::runtime_error("Excepted array object as second argument!");

		uint32_t jsArrLength;
		JS::GetArrayLength(cx,rootedObject1,&jsArrLength);
		std::vector<MH33::GUI::TexturedWidget::TextureCoordDuo> texCoords(jsArrLength / 4);
		for(size_t i = 0; i < texCoords.size(); ++i)
		{
			/*JS::RootedValue rval1(cx);
			JS::RootedValue rval2(cx);
			JS::RootedValue rval3(cx);
			JS::RootedValue rval4(cx);
			JS_GetElement(cx,rootedObject1,(i*4),&rval1);
			JS_GetElement(cx,rootedObject1,(i*4)+1,&rval2);
			JS_GetElement(cx,rootedObject1,(i*4)+2,&rval3);
			JS_GetElement(cx,rootedObject1,(i*4)+3,&rval4);
			texCoords[i].first.x = rval1.toDouble();
			texCoords[i].first.y = rval2.toDouble();
			texCoords[i].second.x = rval3.toDouble();
			texCoords[i].second.y = rval4.toDouble();*/
			texCoords[i].first.x = dumbWorkaround[(i*4)];
			texCoords[i].first.y = dumbWorkaround[(i*4)+1];
			texCoords[i].second.x = dumbWorkaround[(i*4)+2];
			texCoords[i].second.y = dumbWorkaround[(i*4)+3];
		}
		auto objecToInsert = std::make_shared<MH33::GUI::TexturedButton>(texref,texCoords);
		objecToInsert->setNewPosition(glm::fvec2(0.4f,-0.4f),glm::fvec2(0.6f,-0.6f));
		auto objPtr = JS_NewObjectWithGivenProto(cx,&MhTexturedButtonClass.protoClass,*MhTexturedButtonClass.prototype);
		JS::PersistentRootedObject obj(cx, objPtr);
		persistentMap[objecToInsert.get()] = obj;
		objecToInsert->signal_onStateChanged.connect([cx](MH33::GUI::pWidget widg,uint32_t oldState,uint32_t newState) {
			// Ensure the JSContext is valid (assuming cx is a valid JSContext for this thread)
			auto it = persistentMap.find(widg);
			if (it == persistentMap.end()) {
				throw std::runtime_error("Object not found in the persistent map!");
			}
			auto& pers = it->second;
			if (!pers) {
				throw std::runtime_error("Persistent object is null");
			}
			JS::RootedValue rval(cx);
			JS::RootedValue rva2l(cx);
			bool hasProperty;
			JS_HasProperty(cx,pers,"onStateChange",&hasProperty);
			if(!hasProperty) return;
			JS_GetProperty(cx,pers,"onStateChange",&rval);
			if(!rval.isObject()) return;
			if(!JS_ObjectIsFunction(rval.toObjectOrNull())) return;
			JS::RootedFunction rootedFunc(cx, JS_ValueToFunction(cx, rval));
			JS::RootedValueVector valarr(cx);
			valarr.resize(2);
			valarr[0].setInt32(oldState);
			valarr[1].setInt32(newState);
			/*valarr.elements[0].setNumber(reinterpret_cast<intptr_t>(widg));
			valarr.elements[1].setInt32(oldState);
			valarr.elements[2].setInt32(newState);*/
			JS_CallFunction(cx,pers,rootedFunc,valarr,&rva2l);
		});
		// Define a property for the callback, initialized to undefined
		JS::RootedValue undefinedValue(cx, JS::UndefinedValue());
		if (!JS_DefineProperty(cx, obj, "onStateChange", undefinedValue, JSPROP_ENUMERATE | JSPROP_PERMANENT)) {
			return false;
		}
		storeSmartPointerToObj( obj, 0, std::move(objecToInsert));
		args.rval().setObject(*obj);
		return true;
	}, cx, argc, vp);
}

template <MH33::GUI::WidgetStateFlags flag> static bool js_gui_textured_widget_get_flag(JSContext* cx, unsigned argc, JS::Value* vp) {
	/*return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto zobj = JS::getSmartPointerFromObj<MH33::GUI::Widget>(args.thisv().toObjectOrNull(), 0);
		args.rval().setBoolean(zobj->getFlag(flag));
		return true;
	}, cx, argc, vp);*/
	return jsMemberFunctionWrapper(cx,argc,vp,&MH33::GUI::Widget::getFlag<flag>);
};
template <MH33::GUI::WidgetStateFlags flag> static bool js_gui_textured_widget_set_flag(JSContext* cx, unsigned argc, JS::Value* vp) {
	/*return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto zobj = JS::getSmartPointerFromObj<MH33::GUI::Widget>(args.thisv().toObjectOrNull(), 0);
		auto arg0 = args.get(0);
		if(!arg0.isBoolean()) throw std::runtime_error("Excepted boolean as first argument!");
		zobj->setFlag(arg0.toBoolean(),flag);
		args.rval().set(arg0);
		return true;
	}, cx, argc, vp);*/
	return jsMemberFunctionWrapper(cx,argc,vp,&MH33::GUI::Widget::setFlag<flag>);
};
static bool js_gui_textured_widget_get_hidden(JSContext* cx, unsigned argc, JS::Value* vp) {
	/*return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto zobj = JS::getSmartPointerFromObj<MH33::GUI::Widget>(args.thisv().toObjectOrNull(), 0);
		args.rval().setBoolean(zobj->getHidden());
		return true;
	}, cx, argc, vp);*/
	return jsMemberFunctionWrapper(cx,argc,vp,&MH33::GUI::Widget::getHidden);
};
static bool js_gui_textured_widget_set_hidden(JSContext* cx, unsigned argc, JS::Value* vp) {
	/*return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto zobj = JS::getSmartPointerFromObj<MH33::GUI::Widget>(args.thisv().toObjectOrNull(), 0);
		auto arg0 = args.get(0);
		if(!arg0.isBoolean()) throw std::runtime_error("Excepted boolean as first argument!");
		zobj->setHidden(arg0.toBoolean());
		args.rval().set(arg0);
		return true;
	}, cx, argc, vp);*/
	return jsMemberFunctionWrapper(cx,argc,vp,&MH33::GUI::Widget::setHidden);
};
static bool js_gui_textured_widget_addproperty(JSContext* cx, JS::HandleObject obj,
											   JS::HandleId id, JS::HandleValue v) {
	return executeJSAddPropertyOp([](JSContext* cx, JS::HandleObject obj,
								  JS::HandleId id, JS::HandleValue v) {
		return JS_SetPropertyById(cx,obj,id,v);
	}, cx, obj, id, v);
}

void RegisterGuiClasses(JSContext* cx, const HandleObject& global)
{
	if(hasIoBeenInitialized) return;

	// Let's start with the audio buffer

	// AudioBuffer
	std::memset(&MhTexturedButtonClass.protoClass, 0, sizeof(JSClass));
	std::memset(&MhTexturedButtonClass.ops, 0, sizeof(JSClassOps));
	MhTexturedButtonClass.constructor = js_gui_textured_button_constructor;
	MhTexturedButtonClass.name = "TexturedButton";
	MhTexturedButtonClass.nargs = 1;
	MhTexturedButtonClass.ops.finalize = js_finalizer_shared;
	MhTexturedButtonClass.ops.construct = js_gui_textured_button_constructor;
	//MhTexturedButtonClass.ops.addProperty = js_gui_textured_widget_addproperty;
	MhTexturedButtonClass.protoClass.cOps = &MhTexturedButtonClass.ops;
	MhTexturedButtonClass.protoClass.name = "TexturedButton";
	MhTexturedButtonClass.protoClass.flags = JSCLASS_HAS_RESERVED_SLOTS(1) | JSCLASS_BACKGROUND_FINALIZE;
	MhTexturedButtonClass.prototype = std::make_unique<JS::RootedObject>(cx, JS_NewObject(cx,&MhTexturedButtonClass.protoClass));
	MhTexturedButtonClass.ps.push_back(JS_PSGS("hidden", js_gui_textured_widget_get_hidden, js_gui_textured_widget_set_hidden, JSPROP_ENUMERATE ) );
	MhTexturedButtonClass.ps.push_back(JS_PSGS("enabled", js_gui_textured_widget_get_flag<MH33::GUI::WidgetStateFlags::ENABLED>,
											   js_gui_textured_widget_set_flag<MH33::GUI::WidgetStateFlags::ENABLED>, JSPROP_ENUMERATE ) );
	MhTexturedButtonClass.ps.push_back(JS_PSGS("clicked", js_gui_textured_widget_get_flag<MH33::GUI::WidgetStateFlags::CLICKED>,
											   js_gui_textured_widget_set_flag<MH33::GUI::WidgetStateFlags::CLICKED>, JSPROP_ENUMERATE ) );
	MhTexturedButtonClass.ps.push_back(JS_PSGS("inMouseFocus", js_gui_textured_widget_get_flag<MH33::GUI::WidgetStateFlags::IN_MOUSE_FOCUS>,
											   js_gui_textured_widget_set_flag<MH33::GUI::WidgetStateFlags::IN_MOUSE_FOCUS>, JSPROP_ENUMERATE ) );
	MhTexturedButtonClass.ps.push_back(JS_PS_END);
	MhTexturedButtonClass.static_ps.push_back(JS_PS_END);
	MhTexturedButtonClass.fs.push_back(JS_FS_END);
	MhTexturedButtonClass.static_fs.push_back(JS_FS_END);
	{
		auto toReturn = MhTexturedButtonClass.registerToJS(cx, global, *MhTexturedButtonClass.prototype);
		auto reg = JS::RootedValue(cx,JS::ObjectOrNullValue(toReturn) );
		auto rootedReg = JS::RootedObject(cx,toReturn);
		JS_SetProperty(cx, global, MhTexturedButtonClass.name.c_str(), reg );
	}
}

}
