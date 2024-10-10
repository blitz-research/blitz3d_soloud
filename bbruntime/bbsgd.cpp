#include "bbsgd.h"

#include "basic.h"
#include "bbsys.h"

#include <sgd/sgd.h>

namespace {

	void SGD_DECL errorHandler(SGD_String error, void *context) {
		RTEX(error);
	}

	void setConfigVar(BBStr *name, BBStr *value) {
		sgd_SetConfigVar(name->c_str(), value->c_str());
		delete name;
		delete value;
	}

	void alert(BBStr *message) {
		sgd_Alert(message->c_str());
		delete message;
	}

	void createWindow(int width, int height, BBStr *title, int flags) {
		sgd_CreateWindow(width, height, title->c_str(), flags);
		delete title;
	}

	void setWindowTitle(BBStr* title) {
		sgd_SetWindowTitle(title->c_str());
		delete title;
	}

	BBStr* getWindowTitle() {
		return new BBStr(sgd_GetWindowTitle());
	}

	SGD_Texture load2DTexture(BBStr *path, int format, int flags) {
		auto texture = sgd_Load2DTexture(path->c_str(), (SGD_TextureFormat)format, flags);
		delete path;

		return texture;
	}

	SGD_Texture loadArrayTexture(BBStr *path, int format, int flags) {
		auto texture = sgd_LoadArrayTexture(path->c_str(), (SGD_TextureFormat)format, flags);
		delete path;

		return texture;
	}

	SGD_Texture loadCubeTexture(BBStr *path, int format, int flags) {
		auto texture = sgd_LoadCubeTexture(path->c_str(), (SGD_TextureFormat)format, flags);
		delete path;

		return texture;
	}

	SGD_Material loadPBRMaterial(BBStr *path) {
		auto material = sgd_LoadPBRMaterial(path->c_str());
		delete path;

		return material;
	}

	SGD_Material loadPrelitMaterial(BBStr *path) {
		auto material = sgd_LoadPrelitMaterial(path->c_str());
		delete path;

		return material;
	}

	SGD_Mesh SGD_DECL loadMesh(BBStr *path) {
		auto mesh = sgd_LoadMesh(path->c_str());
		delete path;

		return mesh;
	}

	SGD_Entity findEntityChild(SGD_Entity entity, BBStr* name) {
		auto child = sgd_FindEntityChild(entity, name->c_str());
		delete name;

		return child;
	}

	SGD_Skybox loadSkybox(BBStr *path, float roughness) {
		auto skybox = sgd_LoadSkybox(path->c_str(), roughness);
		delete path;

		return skybox;
	}

	SGD_Model loadModel(BBStr *path) {
		auto model = sgd_LoadModel(path->c_str());
		delete path;

		return model;
	}

	SGD_Model loadBonedModel(BBStr *path, int skinned) {
		auto model = sgd_LoadBonedModel(path->c_str(), skinned);
		delete path;

		return model;
	}

	void setMaterialTexture(SGD_Material material, BBStr *name, SGD_Texture texture) {
		sgd_SetMaterialTexture(material, name->c_str(), texture);
		delete name;
	}

	void setMaterialColor(SGD_Material material, BBStr *name, float x, float y, float z, float w) {
		sgd_SetMaterialColor(material, name->c_str(), x, y, z, w);
		delete name;
	}

	void setMaterialFloat(SGD_Material material, BBStr *name, float n) {
		sgd_SetMaterialFloat(material, name->c_str(), n);
		delete name;
	}

	SGD_Font loadFont(BBStr *path, float height) {
		auto font = sgd_LoadFont(path->c_str(), height);
		delete path;

		return font;
	}

	float getTextWidth(SGD_Font font, BBStr *text) {
		auto r = sgd_GetTextWidth(font, text->c_str());
		delete text;
		return r;
	}

	float get2DTextWidth(BBStr *text) {
		auto r = sgd_Get2DTextWidth(text->c_str());
		delete text;
		return r;
	}

	SGD_Image loadImage(BBStr *path) {
		auto r = sgd_LoadImage(path->c_str());
		delete path;
		return r;
	}

	SGD_Image loadArrayImage(BBStr* path, int frameCount, int framesX, int framesY, int frameSpacing) {
		auto r = sgd_LoadArrayImage(path->c_str(), frameCount, framesX, framesY, frameSpacing);
		delete path;
		return r;
	}

	void draw2DText(BBStr *text, float x, float y) {
		sgd_Draw2DText(text->c_str(), x, y);
		delete text;
	}

	SGD_Sound loadSound(BBStr *path) {
		auto sound = sgd_LoadSound(path->c_str());
		delete path;

		return sound;
	}

	BBStr* getEntityName(SGD_Entity entity) {
		return new BBStr(sgd_GetEntityName(entity));
	}

	void loadScene(BBStr* path) {
		sgd_LoadScene(path->c_str());
		delete path;
	}

	void saveScene(BBStr* path) {
		sgd_SaveScene(path->c_str());
		delete path;
	}
}

bool sgd_create() {

	sgd_SetConfigVar("log.stdoutEnabled","0");
	sgd_SetConfigVar("dawn.backendType","D3D11");

	sgd_Init();
	sgd_SetErrorHandler(errorHandler, nullptr);
	return true;
}

bool sgd_destroy() {
	sgd_Terminate();
	return true;
}

bool sgd_link(void (*rtSym)(const char *sym, void *pc)) {

	// System
	rtSym("SetConfigVar$name$value", setConfigVar);
	rtSym("Alert$message", alert);
	rtSym("%GetDesktopWidth", sgd_GetDesktopWidth);
	rtSym("%GetDesktopHeight", sgd_GetDesktopHeight);
	rtSym("%PollEvents", sgd_PollEvents);
	rtSym("ReleaseHandle%handle", sgd_ReleaseHandle);
	rtSym("ReleaseAllHandles", sgd_ReleaseAllHandles);

	// Window
	rtSym("CreateWindow%width%height$title%flags", createWindow);
	rtSym("DestroyWindow", sgd_DestroyWindow);
	rtSym("SetWindowTitle$title", setWindowTitle);
	rtSym("$GetWindowTitle", getWindowTitle);
	rtSym("SetWindowPosition%x%y",sgd_SetWindowPosition);
	rtSym("%GetWindowX", sgd_GetWindowX);
	rtSym("%GetWindowY", sgd_GetWindowY);
	rtSym("SetWindowSize%width%height", sgd_SetWindowSize);
	rtSym("%GetWindowWidth", sgd_GetWindowWidth);
	rtSym("%GetWindowHeight", sgd_GetWindowHeight);
	rtSym("SetFullscreenMode%width%height%refreshRate", sgd_SetFullscreenMode);
	rtSym("SetWindowState%state", sgd_SetWindowState);
	rtSym("%GetWindowState", sgd_GetWindowState);

	// User input
	rtSym("%IsKeyDown%key", sgd_IsKeyDown);
	rtSym("%IsKeyHit%key", sgd_IsKeyHit);
	rtSym("%GetChar", sgd_GetChar);
	rtSym("FlushChars", sgd_FlushChars);
	rtSym("#GetMouseX", sgd_GetMouseX);
	rtSym("#GetMouseY", sgd_GetMouseY);
	rtSym("#GetMouseZ", sgd_GetMouseZ);
	rtSym("#GetMouseVX", sgd_GetMouseVX);
	rtSym("#GetMouseVY", sgd_GetMouseVY);
	rtSym("#GetMouseVZ", sgd_GetMouseVZ);
	rtSym("SetMouseZ#z", sgd_SetMouseZ);
	rtSym("SetMouseCursorMode%cursorMode", sgd_SetMouseCursorMode);
	rtSym("%IsMouseButtonDown%button", sgd_IsMouseButtonDown);
	rtSym("%IsMouseButtonHit%button", sgd_IsMouseButtonHit);
	rtSym("%IsGamepadConnected%gamepad", sgd_IsGamepadConnected);
	rtSym("%IsGamepadButtonDown%gamepad%button", sgd_IsGamepadButtonDown);
	rtSym("%IsGamepadButtonHit%gamepad%button", sgd_IsGamepadButtonHit);
	rtSym("#GetGamepadAxis%gamepad%axis", sgd_GetGamepadAxis);

	// Texture
	rtSym("%Load2DTexture$path%format%flags", load2DTexture);
	rtSym("%LoadArrayTexture$path%format%flags", loadArrayTexture);
	rtSym("%LoadCubeTexture$path%format%flags", loadCubeTexture);
	rtSym("%Create2DTexture%width%height%format%flags", sgd_Create2DTexture);
	rtSym("%CreateArrayTexture%width%height%depth%format%flags", sgd_CreateArrayTexture);
	rtSym("%CreateCubeTexture%size%format%flags", sgd_CreateCubeTexture);
	rtSym("%GetTextureType%texture", sgd_GetTextureType);
	rtSym("%GetTextureWidth%texture", sgd_GetTextureWidth);
	rtSym("%GetTextureHeight%texture", sgd_GetTextureHeight);
	rtSym("%GetTextureDepth%texture", sgd_GetTextureDepth);
	rtSym("%GetTextureWidth%texture", sgd_GetTextureFormat);
	rtSym("%GetTextureWidth%texture", sgd_GetTextureFlags);
	rtSym("SetTexelSRGBA%texture%x%y%rgba", sgd_SetTexelSRGBA);
	rtSym("%GetTexelSRGBA%texture%x%y", sgd_GetTexelSRGBA);

	// Material
	rtSym("%LoadPBRMaterial$path", loadPBRMaterial);
	rtSym("%CreatePBRMaterial", sgd_CreatePBRMaterial);
	rtSym("%LoadPrelitMaterial$path", loadPrelitMaterial);
	rtSym("%CreatePrelitMaterial", sgd_CreatePrelitMaterial);
	rtSym("SetMaterialBlendMode%material%blendMode", sgd_SetMaterialBlendMode);
	rtSym("SetMaterialDepthFunc%material%depthFunc", sgd_SetMaterialDepthFunc);
	rtSym("SetMaterialCullMode%material%cullMode", sgd_SetMaterialCullMode);
	rtSym("SetMaterialTexture%material$name%texture", setMaterialTexture);
	rtSym("SetMaterialColor%material$name#red#green#blue#alpha", setMaterialColor);
	rtSym("SetMaterialFloat%material$name#n", setMaterialFloat);

	// Mesh
	rtSym("%LoadMesh$path", loadMesh);
	rtSym("%CreateSphereMesh#radius%hSegs%vSegs%material", sgd_CreateSphereMesh);
	rtSym("%CreateBoxMesh#minX#minY#minZ#maxX#maxY#maxZ%material", sgd_CreateBoxMesh);
	rtSym("%CreateCylinderMesh#length#radius%segs%material", sgd_CreateCylinderMesh);
	rtSym("%CreateConeMesh#length#radius%segs%material", sgd_CreateConeMesh);
	rtSym("%CreateTorusMesh#outerRadius#innerRadius%outerSegs%innerSegs%material", sgd_CreateTorusMesh);
	rtSym("%CopyMesh%mesh", sgd_CopyMesh);
	rtSym("SetMeshShadowsEnabled%mesh%enabled", sgd_SetMeshShadowsEnabled);
	rtSym("%IsMeshShadowsEnabled%mesh", sgd_IsMeshShadowsEnabled);
	rtSym("UpdateMeshNormals%mesh", sgd_UpdateMeshNormals);
	rtSym("UpdateMeshTangents%mesh", sgd_UpdateMeshTangents);
	rtSym("FitMesh%mesh#minX#minY#minZ#maxX#maxY#maxZ%uniform", sgd_FitMesh);
	rtSym("TransformMesh%mesh#tx#ty#tz#rx#ry#rz#sx#sy#sz", sgd_TransformMesh);
	rtSym("TransformTexCoords%mesh#scaleU#scaleV#offsetU#offsetV", sgd_TransformTexCoords);
	rtSym("%FlipMesh%mesh", sgd_FlipMesh);
	rtSym("#GetMeshBoundsMinX%mesh", sgd_GetMeshBoundsMinX);
	rtSym("#GetMeshBoundsMinY%mesh", sgd_GetMeshBoundsMinY);
	rtSym("#GetMeshBoundsMinZ%mesh", sgd_GetMeshBoundsMinZ);
	rtSym("#GetMeshBoundsMaxX%mesh", sgd_GetMeshBoundsMaxX);
	rtSym("#GetMeshBoundsMaxY%mesh", sgd_GetMeshBoundsMaxY);
	rtSym("#GetMeshBoundsMaxZ%mesh", sgd_GetMeshBoundsMaxZ);

	// Mesh editing - vertices
	rtSym("%CreateMesh%vertexCount%meshFlags", sgd_CreateMesh);
	rtSym("%ResizeVertices%mesh%vertexCount", sgd_ResizeVertices);
	rtSym("%AddVertex%mesh#x#y#z#nx#ny#nz#s#t", sgd_AddVertex);
	rtSym("%GetVertexCount%mesh", sgd_GetVertexCount);
	rtSym("SetVertex%mesh%vertex#x#y#z#nx#ny#nz#s#t", sgd_SetVertex);
	rtSym("SetVertexPosition%mesh%vertex#x#y#z", sgd_SetVertexPosition);
	rtSym("SetVertexNormal%mesh%vertex#nx#ny#nz", sgd_SetVertexNormal);
	rtSym("SetVertexTangent%mesh%vertex#tx#ty#tz#tw", sgd_SetVertexTangent);
	rtSym("SetVertexTexCoord0%mesh%vertex#u0#v0", sgd_SetVertexTexCoord0);
	rtSym("SetVertexColor%mesh%vertex#r#g#b#a", sgd_SetVertexColor);
	rtSym("#GetVertexX%mesh%vertex", sgd_GetVertexX);
	rtSym("#GetVertexY%mesh%vertex", sgd_GetVertexY);
	rtSym("#GetVertexZ%mesh%vertex", sgd_GetVertexZ);
	rtSym("#GetVertexNX%mesh%vertex", sgd_GetVertexNX);
	rtSym("#GetVertexNY%mesh%vertex", sgd_GetVertexNY);
	rtSym("#GetVertexNZ%mesh%vertex", sgd_GetVertexNZ);
	rtSym("#GetVertexTX%mesh%vertex", sgd_GetVertexTX);
	rtSym("#GetVertexTY%mesh%vertex", sgd_GetVertexTY);
	rtSym("#GetVertexTZ%mesh%vertex", sgd_GetVertexTZ);
	rtSym("#GetVertexTW%mesh%vertex", sgd_GetVertexTW);
	rtSym("#GetVertexRed%mesh%vertex", sgd_GetVertexRed);
	rtSym("#GetVertexGreen%mesh%vertex", sgd_GetVertexGreen);
	rtSym("#GetVertexBlue%mesh%vertex", sgd_GetVertexBlue);
	rtSym("#GetVertexAlpha%mesh%vertex", sgd_GetVertexAlpha);
	rtSym("#GetVertexU0%mesh%vertex", sgd_GetVertexU0);
	rtSym("#GetVertexV0%mesh%vertex", sgd_GetVertexV0);

	// Surfaces
	rtSym("%CreateSurface%mesh%triangleCount%material", sgd_CreateSurface);
	rtSym("%GetSurfaceCount%mesh", sgd_GetSurfaceCount);
	rtSym("%GetSurface%mesh%surface", sgd_GetSurface);
	rtSym("%GetMaterial%surface", sgd_GetMaterial);
	rtSym("%ResizeTriangles%surface%triangleCount", sgd_ResizeTriangles);
	rtSym("%AddTriangle%surface%vertex0%vertex1%vertex2", sgd_AddTriangle);
	rtSym("%GetTriangleCount%surface", sgd_GetTriangleCount);
	rtSym("SetTriangle%surface%triangle%vertex0%vertex1%vertex2", sgd_SetTriangle);
	rtSym("%GetTriangleVertex%surface%triangle%vertex", sgd_GetTriangleVertex);

	// Font
	rtSym("%LoadFont$path#height", loadFont);
	rtSym("#GetTextWidth%font$text", getTextWidth);
	rtSym("#GetFontHeight%font", sgd_GetFontHeight);

	// Image
	rtSym("%LoadImage$path", loadImage);
	rtSym("%LoadArrayImage$path%frameCount%framesX%framesY%frameSpacing", loadArrayImage);
	rtSym("%CreateImage%texture", sgd_CreateImage);
	rtSym("SetImageViewMode%image%viewMode", sgd_SetImageViewMode);
	rtSym("SetImageBlendMode%image%blendMode", sgd_SetImageBlendMode);
	rtSym("SetImageRect%image#minX#minY#maxX#maxY", sgd_SetImageRect);
	rtSym("%GetImagetexture%image", sgd_GetImageTexture);

	// 2D Overlay state
	rtSym("Set2DFillColor#red#green#blue#alpha", sgd_Set2DFillColor);
	rtSym("Set2DFillMaterial%material", sgd_Set2DFillMaterial);
	rtSym("Set2DFillEnabled%enabled", sgd_Set2DFillEnabled);
	rtSym("Set2DOutlineColor#red#green#blue#alpha", sgd_Set2DOutlineColor);
	rtSym("Set2DOutlineWidth#width", sgd_Set2DOutlineWidth);
	rtSym("Set2DOutlineEnabled%enabled", sgd_Set2DOutlineEnabled);
	rtSym("Set2DLineWidth#width", sgd_Set2DLineWidth);
	rtSym("Set2DPointSize#size", sgd_Set2DPointSize);
	rtSym("Set2DFont%font", sgd_Set2DFont);
	rtSym("Set2DTextColor#red#green#blue#alpha", sgd_Set2DTextColor);
	rtSym("#Get2DTextWidth$text", get2DTextWidth);
	rtSym("#Get2DFontHeight", sgd_Get2DFontHeight);

	// 2D Overlay drawing
	rtSym("Clear2D", sgd_Clear2D);
	rtSym("Push2DLayer", sgd_Push2DLayer);
	rtSym("Pop2DLayer", sgd_Pop2DLayer);
	rtSym("Draw2DPoint#x#y", sgd_Draw2DPoint);
	rtSym("Draw2DLine#x0#y0#x1#y1", sgd_Draw2DLine);
	rtSym("Draw2DRect#minX#minY#maxX#maxY", sgd_Draw2DRect);
	rtSym("Draw2DOval#minX#minY#maxX#maxY", sgd_Draw2DOval);
	rtSym("Draw2DImage%image#x#y#frame", sgd_Draw2DImage);
	rtSym("Draw2DText$text#x#y", draw2DText);

	// Scene
	rtSym("ClearScene", sgd_ClearScene);
	rtSym("ResetScene%releaseAllHandles", sgd_ResetScene);
	rtSym("LoadScene$path", loadScene);
	rtSym("SaveScene$path", saveScene);
	rtSym("SetAmbientLightColor#red#green#blue#alpha", sgd_SetAmbientLightColor);
	rtSym("SetClearColor#red#green#blue#alpha", sgd_SetClearColor);
	rtSym("SetClearDepth#depth", sgd_SetClearDepth);
	rtSym("SetEnvTexture%texture", sgd_SetEnvTexture);
	rtSym("UpdateShadowMappingConfig", sgd_UpdateShadowMappingConfig);
	rtSym("RenderScene", sgd_RenderScene);
	rtSym("Present", sgd_Present);
	rtSym("#GetFPS", sgd_GetFPS);
	rtSym("#GetRPS", sgd_GetRPS);

	// Entity
	rtSym("SetEntityEnabled%entity%enabled", sgd_SetEntityEnabled);
	rtSym("%IsEntityEnabled%entity", sgd_IsEntityEnabled);
	rtSym("SetEntityVisible%entity%visible", sgd_SetEntityVisible);
	rtSym("%IsEntityVisible%entity", sgd_IsEntityVisible);
	rtSym("DestroyEntity%entity", sgd_DestroyEntity);
	rtSym("%CopyEntity%entity", sgd_CopyEntity);
	rtSym("ResetEntity%entity", sgd_ResetEntity);
	rtSym("SetEntityName%entity$name", sgd_SetEntityName);
	rtSym("$GetEntityName%entity", getEntityName);
	rtSym("SetEntityParent%entity%parent", sgd_SetEntityParent);
	rtSym("%GetEntityParent%entity", sgd_GetEntityParent);
	rtSym("%GetEntityChildCount%entity", sgd_GetEntityChildCount);
	rtSym("%GetEntityChild%entity%childIndex", sgd_GetEntityChild);
	rtSym("%FindEntityChild%entity$childName", findEntityChild);
	rtSym("SetEntityPosition%entity#x#y#z", sgd_SetEntityPosition);
	rtSym("SetEntityRotation%entity#rx#ry#rz", sgd_SetEntityRotation);
	rtSym("SetEntityScale%entity#sx#sy#sz", sgd_SetEntityScale);
	rtSym("TranslateEntity%entity#x#y#z", sgd_TranslateEntity);
	rtSym("RotateEntity%entity#rx#ry#rz", sgd_RotateEntity);
	rtSym("ScaleEntity%entity#sx#sy#sz", sgd_ScaleEntity);
	rtSym("MoveEntity%entity#x#y#z", sgd_MoveEntity);
	rtSym("TurnEntity%entity#rx#ry#rz", sgd_TurnEntity);
	rtSym("#GetEntityX%entity", sgd_GetEntityX);
	rtSym("#GetEntityY%entity", sgd_GetEntityY);
	rtSym("#GetEntityZ%entity", sgd_GetEntityZ);
	rtSym("#GetEntityRX%entity", sgd_GetEntityRX);
	rtSym("#GetEntityRY%entity", sgd_GetEntityRY);
	rtSym("#GetEntityRZ%entity", sgd_GetEntityRZ);
	rtSym("#GetEntitySX%entity", sgd_GetEntitySX);
	rtSym("#GetEntitySY%entity", sgd_GetEntitySY);
	rtSym("#GetEntitySZ%entity", sgd_GetEntitySZ);
	rtSym("#GetEntityIX%entity", sgd_GetEntityIX);
	rtSym("#GetEntityIY%entity", sgd_GetEntityIY);
	rtSym("#GetEntityIZ%entity", sgd_GetEntityIZ);
	rtSym("#GetEntityJX%entity", sgd_GetEntityJX);
	rtSym("#GetEntityJY%entity", sgd_GetEntityJY);
	rtSym("#GetEntityJZ%entity", sgd_GetEntityJZ);
	rtSym("#GetEntityKX%entity", sgd_GetEntityKX);
	rtSym("#GetEntityKY%entity", sgd_GetEntityKY);
	rtSym("#GetEntityKZ%entity", sgd_GetEntityKZ);
	rtSym("AimEntityAtEntity%entity%target#roll", sgd_AimEntityAtEntity);
	rtSym("AimEntityAtPoint%entity#x#y#z#roll", sgd_AimEntityAtPoint);
	rtSym("TransformPoint#x#y#z%srcEntity%dstEntity", sgd_TransformPoint);
	rtSym("TransformVector#x#y#z%srcEntity%dstEntity", sgd_TransformVector);
	rtSym("TransformNormal#x#y#z%srcEntity%dstEntity", sgd_TransformNormal);
	rtSym("#GetTransformedX", sgd_GetTransformedX);
	rtSym("#GetTransformedY", sgd_GetTransformedY);
	rtSym("#GetTransformedZ", sgd_GetTransformedZ);

	// Camera
	rtSym("%CreatePerspectiveCamera", sgd_CreatePerspectiveCamera);
	rtSym("%CreateOrthographicCamera", sgd_CreateOrthographicCamera);
	rtSym("%GetCameraType%camera", sgd_GetCameraType);
	//
	rtSym("SetCameraFOV%camera#fov", sgd_SetCameraFOV);
	rtSym("SetCameraNear%camera#near", sgd_SetCameraNear);
	rtSym("SetCameraFar%camera#far", sgd_SetCameraFar);
	//
	rtSym("%CameraProject%camera#x#y#z", sgd_CameraProject);
	rtSym("#GetProjectedX", sgd_GetProjectedX);
	rtSym("#GetProjectedY", sgd_GetProjectedY);
	rtSym("%CameraUnproject%camera#windowX#windowY#depth", sgd_CameraUnproject);
	rtSym("#GetUnprojectedX", sgd_GetUnprojectedX);
	rtSym("#GetUnprojectedY", sgd_GetUnprojectedY);
	rtSym("#GetUnprojectedZ", sgd_GetUnprojectedZ);

	// Light
	rtSym("%CreateDirectionalLight", sgd_CreateDirectionalLight);
	rtSym("%CreatePointLight", sgd_CreatePointLight);
	rtSym("%CreateSpotLight", sgd_CreateSpotLight);
	rtSym("%GetLightType%light", sgd_GetLightType);
	rtSym("SetLightShadowsEnabled%light%enabled", sgd_SetLightShadowsEnabled);
	rtSym("%IsLightShadowsEnabled%light", sgd_IsLightShadowsEnabled);
	rtSym("SetLightPriority%light%priority", sgd_SetLightPriority);
	rtSym("%GetLightPriority%light", sgd_GetLightPriority);
	//
	rtSym("SetLightColor%light#red#green#blue#alpha", sgd_SetLightColor);
	rtSym("SetLightRange%light#range", sgd_SetLightRange);
	rtSym("SetLightFalloff%light#falloff", sgd_SetLightFalloff);
	rtSym("SetLightInnerConeAngle%light#angle", sgd_SetLightInnerConeAngle);
	rtSym("SetLightOuterConeAngle%light#angle", sgd_SetLightOuterConeAngle);

	// Model
	rtSym("%LoadModel$path", loadModel);
	rtSym("%LoadBonedModel$path%skinned", loadBonedModel);
	rtSym("%CreateModel%mesh", sgd_CreateModel);
	rtSym("SetModelMesh%model%mesh", sgd_SetModelMesh);
	rtSym("%GetModelMesh%model", sgd_GetModelMesh);
	rtSym("SetModelColor%model#red#green#blue#alpha", sgd_SetModelColor);
	rtSym("AnimateModel%model%animation#time%mode#weight", sgd_AnimateModel);

	// Skybox
	rtSym("%LoadSkybox$path#roughness", loadSkybox);
	rtSym("%CreateSkybox%texture", sgd_CreateSkybox);
	rtSym("SetSkyboxTexture%skybox%texture", sgd_SetSkyboxTexture);
	rtSym("SetSkyboxRoughness%skybox#roughness", sgd_SetSkyboxRoughness);

	// Terrain
	rtSym("%CreateTerrain", sgd_CreateTerrain);
	rtSym("SetTerrainSize%terrain%size", sgd_SetTerrainSize);
	rtSym("SetTerrainLODs%terrain%lods", sgd_SetTerrainLODs);
	rtSym("SetTerrainMaterial%terrain%material", sgd_SetTerrainMaterial);
	rtSym("SetTerrainMaterialSize%terrain%materialSize", sgd_SetTerrainMaterialSize);
	rtSym("SetTerrainHeightTexture%terrain%texture", sgd_SetTerrainHeightTexture);
	rtSym("SetTerrainNormalTexture%terrain%texture", sgd_SetTerrainNormalTexture);
	rtSym("SetTerrainDebugMode%terrain%debugMode", sgd_SetTerrainDebugMode);

	// Sprite
	rtSym("%CreateSprite%image", sgd_CreateSprite);
	rtSym("SetSpriteImage%sprite%image", sgd_SetSpriteImage);
	rtSym("SetSpriteColor%sprite#red#green#blue#alpha", sgd_SetSpriteColor);
	rtSym("SetSpriteFrame%sprite#frame", sgd_SetSpriteFrame);

	// Collisions
	rtSym("%CreateSphereCollider%entity%colliderType#radius", sgd_CreateSphereCollider);
	rtSym("%CreateEllipsoidCollider%entity%colliderType#radius#height", sgd_CreateEllipsoidCollider);
	rtSym("%CreateMeshCollider%entity%colliderType%mesh", sgd_CreateMeshCollider);
	rtSym("%GetColliderEntity%collider", sgd_GetColliderEntity);
	rtSym("%GetColliderType%collider", sgd_GetColliderType);
	rtSym("SetColliderRadius%collider#radius", sgd_SetColliderRadius);
	rtSym("SetColliderHeight%collider#height", sgd_SetColliderHeight);
	rtSym("EnableCollisions%srcColliderType%dstColliderType%collisionResponse", sgd_EnableCollisions);
	rtSym("UpdateColliders", sgd_UpdateColliders);
	rtSym("%GetCollisionCount%collider", sgd_GetCollisionCount);
	rtSym("%GetCollisionCollider%collider%index", sgd_GetCollisionCollider);
	rtSym("#GetCollisionX%collider%index", sgd_GetCollisionX);
	rtSym("#GetCollisionY%collider%index", sgd_GetCollisionY);
	rtSym("#GetCollisionZ%collider%index", sgd_GetCollisionZ);
	rtSym("#GetCollisionNX%collider%index", sgd_GetCollisionNX);
	rtSym("#GetCollisionNY%collider%index", sgd_GetCollisionNY);
	rtSym("#GetCollisionNZ%collider%index", sgd_GetCollisionNZ);

	// Picking
	rtSym("%CameraPick%camera#windowX#windowY%colliderMask", sgd_CameraPick);
	rtSym("%LinePick#x0#y0#z0#x1#y1#z1#radius%colliderMask", sgd_LinePick);
	rtSym("#GetPickedX", sgd_GetPickedX);
	rtSym("#GetPickedY", sgd_GetPickedY);
	rtSym("#GetPickedZ", sgd_GetPickedZ);
	rtSym("#GetPickedNX", sgd_GetPickedNX);
	rtSym("#GetPickedNY", sgd_GetPickedNY);
	rtSym("#GetPickedNZ", sgd_GetPickedNZ);

	// Render effects
	rtSym("%CreateBloomEffect", sgd_CreateBloomEffect);
	rtSym("SetBloomEffectRadius%effect%radius", sgd_SetBloomEffectRadius);
	rtSym("%GetBloomEffectRadius%effect", sgd_GetBloomEffectRadius);
	rtSym("%CreateBlurEffect", sgd_CreateBlurEffect);
	rtSym("SetBlurEffectRadius%effect%radius", sgd_SetBlurEffectRadius);
	rtSym("%GetBlurEffectRadius%effect", sgd_GetBlurEffectRadius);
	rtSym("%CreateMonocolorEffect", sgd_CreateMonocolorEffect);
	rtSym("SetMonocolorEffectColor%effect#red#green#blue#alpha", sgd_SetMonocolorEffectColor);
	rtSym("%CreateFogEffect", sgd_CreateFogEffect);
	rtSym("SetFogEffectColor%effect#red#green#blue#alpha", sgd_SetFogEffectColor);
	rtSym("SetFogEffectRange%effect#near#far", sgd_SetFogEffectRange);
	rtSym("SetFogEffectPower%effect#power", sgd_SetFogEffectPower);
	rtSym("SetRenderEffectEnabled%effect%enabled", sgd_SetRenderEffectEnabled);
	rtSym("%IsRenderEffectEnabled%effect", sgd_IsRenderEffectEnabled);

	// Audio
	rtSym("%LoadSound$path", loadSound);
	rtSym("%PlaySound%sound", sgd_PlaySound);
	rtSym("%CueSound%sound", sgd_CueSound);
	rtSym("SetAudioVolume%audio#volume", sgd_SetAudioVolume);
	rtSym("SetAudioPan%audio#pan", sgd_SetAudioPan);
	rtSym("SetAudioPitchScale%audio#scale", sgd_SetAudioPitchScale);
	rtSym("SetAudioLooping%audio%looping", sgd_SetAudioLooping);
	rtSym("SetAudioPaused%audio%paused", sgd_SetAudioPaused);
	rtSym("%IsAudioValid%audio", sgd_IsAudioValid);
	rtSym("StopAudio%audio", sgd_StopAudio);
	rtSym("Set3DAudioConfig#rolloffScale#dopplerScale#distanceScale", sgd_Set3DAudioConfig);
	rtSym("%Create3DAudioListener%entity", sgd_Create3DAudioListener);
	rtSym("%Play3DSound%sound%entity", sgd_Play3DSound);
	rtSym("Update3DAudio", sgd_Update3DAudio);

	return true;
}
