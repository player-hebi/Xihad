/**
 * TODO 
 * 1. Affector ����������
 */
#include "luaopen_ParticleSystem.h"
#include "IrrlichtParticleSystems.h"
#include "RangeSetterFunction.h"
#include "CWrappedMeshInitializer.h"
#include "IParticleSystemScriptFactory.h"
#include "IStackPusher.h"
#include <luaT/luaT.h>
#include <irrlicht/ISceneManager.h>
#include <CppBase/StringUtil.h>
#include "IParticleSystemLoaderEnv.h"
#include "CWrappedAnimatedMeshInitializer.h"

using namespace irr;
using namespace scene;
using namespace video;
using namespace luaT;

#define setRangeFunc(field)	{ "set"###field, rangeSetter<field> }

namespace {		/// strings for template parameter
	char PPS[] = "PPS";
	char Color[] = "Color";
	char LifeTime[] = "LifeTime";
	char Width[] = "Width";
	char Height[] = "Height";
	char NormalDirectionSize[] = "NormalDirectionSize";
}

namespace xihad { namespace particle
{
	//////////////////////////////////////////////////////////////////////
	/// Particle System Scene Node
	////////////////////////////////////////////////////////////////////// 
	static int addEmitter(lua_State* L)
	{
		luaT_variable(L, 1, IParticleSystemSceneNode*, pnode);
		luaT_variable(L, 2, IParticleEmitter*, e);

		if (lua_gettop(L) < 3)
			pnode->addEmitter(e);
		else if (lua_gettop(L) < 4)
			pnode->addEmitter(e, (f32)lua_tonumber(L, 3));
		else
			pnode->addEmitter(e, (f32)lua_tonumber(L, 3), (f32)lua_tonumber(L, 4));

		e->drop();
		return 0;
	}

	static int addAffector(lua_State* L)
	{
		luaT_variable(L, 1, IParticleSystemSceneNode*, pnode);
		luaT_variable(L, 2, IParticleAffector*, a);

		if (!a)
			luaL_typerror(L, 2, "Got null affector!");

		f32 ratioBegin = 0.f, ratioEnd = 1.f, globalBgn = 0.f, globalEnd = 16000000.f;
		if (lua_gettop(L) >= 3)
			ratioBegin = (f32)lua_tonumber(L, 3);

		if (lua_gettop(L) >= 4)
			ratioEnd = (f32)lua_tonumber(L, 4);

		if (lua_gettop(L) >= 5)
			globalBgn = (f32)lua_tonumber(L, 5);

		if (lua_gettop(L) >= 6)
			globalEnd = (f32)lua_tonumber(L, 6);


		pnode->addAffector(a, ratioBegin, ratioEnd, globalBgn, globalEnd);
		a->drop();
		return 0;
	}

	luaT_static void setRenderer(IParticleSystemSceneNode* pnode, IParticleRenderer* render)
	{
		pnode->setRenderer(render);
		render->drop();
	}}

	luaT_static void setRendererTexture(IParticleSystemSceneNode* pnode, 
		IParticleRenderer* render, int layer, const char* texPath)
	{
		ITexture* tex = pnode->getSceneManager()->getVideoDriver()->getTexture(texPath);
		render->getMaterial().setTexture(layer, tex);
	}}

	static int newChild(lua_State* L)
	{
		int argc = lua_gettop(L);
		luaT_variable(L, 1, IParticleSystemSceneNode*, pnode);
		core::vector3df position = argc>=2 ? 
			luaT::checkarg<core::vector3df>(L, 2) : core::vector3df();
		core::vector3df rotation = argc>=3 ?
			luaT::checkarg<core::vector3df>(L, 3) : core::vector3df();

		ISceneManager* smgr = pnode->getSceneManager();
		push<IParticleSystemSceneNode*>(L, 
			smgr->addParticleSystemSceneNode(65535, pnode, -1, position, rotation));

		return 1;
	}

	void luaopen_ParticleSystemNode( lua_State* L )
	{
		MetatableFactory<ISceneNode>::create(L, 0);

		luaT_defRegsBgn(pnode)
			luaT_mnamedfunc(IParticleSystemSceneNode, setParticlesAreGlobal),
			luaT_mnamedfunc(IParticleSystemSceneNode, setMaxParticleCount),
			luaT_lnamedfunc(addEmitter),
			luaT_lnamedfunc(addAffector),
			luaT_cnamedfunc(setRenderer),
			luaT_cnamedfunc(setRendererTexture),
			luaT_lnamedfunc(newChild),
		luaT_defRegsEnd
		MetatableFactory<IParticleSystemSceneNode, ISceneNode>::create(L, pnode);

		luaT_defRegsBgn(env)
			luaT_mnamedfunc(IParticleSystemLoaderEnv, setPosition),
			luaT_mnamedfunc(IParticleSystemLoaderEnv, getPosition),
			luaT_mnamedfunc(IParticleSystemLoaderEnv, getAABB),
			luaT_mnamedfunc(IParticleSystemLoaderEnv, getMesh),
			luaT_mnamedfunc(IParticleSystemLoaderEnv, getNode),
		luaT_defRegsEnd
		MetatableFactory<IParticleSystemLoaderEnv>::create(L, env);
	}

	//////////////////////////////////////////////////////////////////////
	/// Particle System Factory
	////////////////////////////////////////////////////////////////////// 
	template <IStackPusher* (IParticleSystemScriptFactory::* mfunc)(const char*)>
	static int ladapter(lua_State* L)
	{
		luaT_variable(L, 1, IParticleSystemScriptFactory*, factory);
		const char* pname = lua_tostring(L, 2);
		pname = pname ? pname : "";
		IStackPusher* pusher = (factory->*mfunc)(pname);
		if (pusher)
		{
			pusher->pushOnto(L);
			pusher->drop();
			return 1;
		}

		luaL_argerror(L, 2, "Creation parameter error, no particle component created");
		return 0;
	}

	static int initer(lua_State* L)
	{
		if (lua_istable(L, 2))
			return luaT::checkarg<IParticleSystemScriptFactory*>(L, 1)->compositeIniter(L);
		else if (lua_isstring(L, 2))
			return ladapter<&IParticleSystemScriptFactory::initer>(L);
		else
			luaL_typerror(L, 2, "table/string");

		return 0;
	}

	void luaopen_ParticleSystemFactory( lua_State* L )
	{
		luaT_defRegsBgn(factory)
			luaT_lnamedfunc(initer),
			luaT_lnnamefunc(ladapter<&IParticleSystemScriptFactory::emitter>, emitter),
			luaT_lnnamefunc(ladapter<&IParticleSystemScriptFactory::renderer>, renderer),
			luaT_lnnamefunc(ladapter<&IParticleSystemScriptFactory::affector>, affector),
		luaT_defRegsEnd
		MetatableFactory<IParticleSystemScriptFactory>::create(L, factory);
	}

	//////////////////////////////////////////////////////////////////////
	/// Particle Emitter
	////////////////////////////////////////////////////////////////////// 
	luaT_static void grab(IParticleEmitter* e)
	{
		e->grab();
	}}

	luaT_static void setIniter(IParticleEmitter* emitter,  IParticleInitializer* initer) 
	{
		emitter->setParticleInitializer(initer);
		initer->drop();
	}}

	void luaopen_ParticleEmitter( lua_State* L )
	{
		luaT_defRegsBgn(emitter)
			luaT_cnamedfunc(grab),
			luaT_cnamedfunc(setIniter),
			luaT_mnnamefunc(IParticleEmitter, setMinParticlesPerSecond, setMinPPS),
			luaT_mnnamefunc(IParticleEmitter, setMaxParticlesPerSecond, setMaxPPS),
			setRangeFunc(PPS),
		luaT_defRegsEnd
		MetatableFactory<IParticleEmitter>::create(L, emitter);
	}


	//////////////////////////////////////////////////////////////////////
	/// Particle Renderer
	////////////////////////////////////////////////////////////////////// 
	luaT_static void setMaterialType(SMaterial& mat, const char* type)
	{
		int idx = StringUtil::select(type, sBuiltInMaterialTypeNames);
		if (idx != -1)
			mat.MaterialType = (E_MATERIAL_TYPE) idx;
		else 
			std::cerr << "Unrecognized material type: "  << type << std::endl;
	}}

	luaT_static void setLighting(SMaterial& mat, bool b)
	{
		mat.Lighting = b;
	}}

	luaT_static void setZWriteEnable(SMaterial& mat, bool b)
	{
		mat.ZWriteEnable = b;
	}}

	luaT_static void setBlend(SMaterial& mat, const char* fsrc, const char* fdst, const char* bops)
	{
		static const char* sBlendFactors[] = {
			"0", "1", "dst_color", "1 - dst_color", "src_color", "1 - src_color",
			"src_alpha", "1 - src_alpha", "dst_alpha", "1 - dst_alpha", 0
		};
		static const char* sBlendOperation[] = {
			"none", "add", "substract", "rev_substract", "min", "max", 0
		};
		
		int src = StringUtil::select(fsrc, sBlendFactors);
		int dst = StringUtil::select(fdst, sBlendFactors);
		if (src != -1 && dst != -1)
			mat.MaterialTypeParam = pack_textureBlendFunc((E_BLEND_FACTOR) src, (E_BLEND_FACTOR) dst);
		else if (src == -1)
			std::cerr << "Unrecognized src blend factor: " << fsrc << std::endl;
		else 
			std::cerr << "Unrecognized dst blend factor: " << fdst << std::endl;

		int ops = StringUtil::select(bops, sBlendOperation);
		if (ops != -1)
			mat.BlendOperation = (E_BLEND_OPERATION) ops;
		else 
			std::cerr << "Unrecognized blend operation: " << bops << std::endl;
	}}

	luaT_static void addTexRegion(IParticleQuadRenderer* quad, f32 x, f32 y, f32 x2, f32 y2)
	{
		quad->addTextureRegion(core::rectf(x,y,x2,y2));
	}}

	void luaopen_ParticleRenderer( lua_State* L )
	{
		luaT_defRegsBgn(smat)
			luaT_cnamedfunc(setMaterialType),
			luaT_cnamedfunc(setBlend),
			luaT_cnamedfunc(setLighting),
			luaT_cnamedfunc(setZWriteEnable),
		luaT_defRegsEnd
		MetatableFactory<SMaterial>::create(L, smat);

		luaT_defRegsBgn(renderer)
			luaT_mnamedfunc(IParticleRenderer, getMaterial),
		luaT_defRegsEnd
		MetatableFactory<IParticleRenderer>::create(L, renderer);

		luaT_defRegsBgn(point_render)
			luaT_mnamedfunc(IParticlePointRenderer, setPointSize),
		luaT_defRegsEnd
		MetatableFactory<IParticlePointRenderer, IParticleRenderer>::create(L, point_render);

		luaT_defRegsBgn(quad_render)
			luaT_cnamedfunc(addTexRegion),
			luaT_mnamedfunc(IParticleQuadRenderer, setOrintation),
		luaT_defRegsEnd
		MetatableFactory<IParticleQuadRenderer, IParticleRenderer>::create(L, quad_render);
	}


	//////////////////////////////////////////////////////////////////////
	/// Particle Initializer
	////////////////////////////////////////////////////////////////////// 
	luaT_static void setMinColor(IParticleBasicInitializer* init, u32 clr)
	{
		init->setMinStartColor(video::SColor(clr));
	}}

	luaT_static void setMaxColor(IParticleBasicInitializer* init, u32 clr)
	{
		init->setMaxStartColor(video::SColor(clr));
	}}

	luaT_static void setMinWidth(IParticleBasicInitializer* init, f32 minW)
	{
		core::dimension2df size = init->getMinStartSize();
		size.Width = minW;
		init->setMinStartSize(size);
	}}

	luaT_static void setMaxWidth(IParticleBasicInitializer* init, f32 maxW)
	{
		core::dimension2df size = init->getMaxStartSize();
		size.Width = maxW;
		init->setMaxStartSize(size);
	}}

	luaT_static void setMinHeight(IParticleBasicInitializer* init, f32 minH)
	{
		core::dimension2df size = init->getMinStartSize();
		size.Height = minH;
		init->setMinStartSize(size);
	}}

	luaT_static void setMaxHeight(IParticleBasicInitializer* init, f32 maxH)
	{
		core::dimension2df size = init->getMaxStartSize();
		size.Height = maxH;
		init->setMaxStartSize(size);
	}}

	void luaopen_ParticleIniter( lua_State* L )
	{
		MetatableFactory<IParticleInitializer>::create(L, 0);

		luaT_defRegsBgn(basic_initer)
			luaT_cnamedfunc(setMinColor),
			luaT_cnamedfunc(setMaxColor),
			setRangeFunc(Color),
			luaT_mnamedfunc(IParticleBasicInitializer, setMinLifeTime),
			luaT_mnamedfunc(IParticleBasicInitializer, setMaxLifeTime),
			setRangeFunc(LifeTime),
			luaT_cnamedfunc(setMinWidth),
			luaT_cnamedfunc(setMaxWidth),
			setRangeFunc(Width),
			luaT_cnamedfunc(setMinHeight),
			luaT_cnamedfunc(setMaxHeight),
			setRangeFunc(Height),
		luaT_defRegsEnd
		MetatableFactory<IParticleBasicInitializer, IParticleInitializer>::create(L, basic_initer);

		luaT_defRegsBgn(direction_initer)
			luaT_mnamedfunc(IParticleDirectionInitializer, setMaxAngleDegree),
			luaT_mnamedfunc(IParticleDirectionInitializer, setDirection),
		luaT_defRegsEnd
		MetatableFactory<IParticleDirectionInitializer, IParticleInitializer>::create(L, direction_initer);

		luaT_defRegsBgn(geo_initer)
			luaT_mnamedfunc(IParticleGeometricInitializer, setMinNormalDirectionSize),
			luaT_mnamedfunc(IParticleGeometricInitializer, setMaxNormalDirectionSize),
			setRangeFunc(NormalDirectionSize),
			luaT_mnamedfunc(IParticleGeometricInitializer, setOutlineOnly),
		luaT_defRegsEnd
		MetatableFactory<IParticleGeometricInitializer, IParticleInitializer>::create(L, geo_initer);

		luaT_defRegsBgn(wmesh_initer)
			luaT_mnamedfunc(CWrappedMeshInitializer, setMinNormalDirectionSize),
			luaT_mnamedfunc(CWrappedMeshInitializer, setMaxNormalDirectionSize),
			setRangeFunc(NormalDirectionSize),
			luaT_mnamedfunc(CWrappedMeshInitializer, setOutlineOnly),
			luaT_mnamedfunc(CWrappedMeshInitializer, setMesh),
		luaT_defRegsEnd
		MetatableFactory<CWrappedMeshInitializer, IParticleInitializer>::create(L, wmesh_initer);

		luaT_defRegsBgn(wamesh_initer)
			luaT_mnamedfunc(CWrappedAnimatedMeshInitializer, setMinNormalDirectionSize),
			luaT_mnamedfunc(CWrappedAnimatedMeshInitializer, setMaxNormalDirectionSize),
			setRangeFunc(NormalDirectionSize),
			luaT_mnamedfunc(CWrappedAnimatedMeshInitializer, setOutlineOnly),
			luaT_mnamedfunc(CWrappedAnimatedMeshInitializer, setAnimatedNode),
		luaT_defRegsEnd
		MetatableFactory<CWrappedAnimatedMeshInitializer, IParticleInitializer>::create(L, wamesh_initer);


		luaT_defRegsBgn(box_initer)
			luaT_mnamedfunc(IParticleBoxInitializer, setBox),
		luaT_defRegsEnd
		MetatableFactory<IParticleBoxInitializer, IParticleGeometricInitializer>::create(L, box_initer);

		luaT_defRegsBgn(sph_initer)
			luaT_mnamedfunc(IParticleSphereInitializer, setCenter),
			luaT_mnamedfunc(IParticleSphereInitializer, setRadius),
		luaT_defRegsEnd
		MetatableFactory<IParticleSphereInitializer, IParticleGeometricInitializer>::create(L, sph_initer);

		luaT_defRegsBgn(cyl_initer)
			luaT_mnamedfunc(IParticleCylinderInitializer, setCenter),
			luaT_mnamedfunc(IParticleCylinderInitializer, setRadius),
			luaT_mnamedfunc(IParticleCylinderInitializer, setNormal),
		luaT_defRegsEnd
		MetatableFactory<IParticleCylinderInitializer, IParticleGeometricInitializer>::create(L, cyl_initer);

		luaT_defRegsBgn(ring_initer)
			luaT_mnamedfunc(IParticleRingInitializer, setCenter),
			luaT_mnamedfunc(IParticleRingInitializer, setRadius),
			luaT_mnnamefunc(IParticleRingInitializer, setRingThickness, setThickness),
		luaT_defRegsEnd
		MetatableFactory<IParticleRingInitializer, IParticleGeometricInitializer>::create(L, ring_initer);

		MetatableFactory<IParticleCompositeInitializer, IParticleInitializer>::create(L, 0);
	}


	//////////////////////////////////////////////////////////////////////
	/// Particle Affector
	////////////////////////////////////////////////////////////////////// 
	luaT_static void setTargetColor(IParticleFadeOutAffector* faff, u32 clr)
	{
		faff->setTargetColor(video::SColor(clr));
	}}

	luaT_static void setColorMask(IParticleFadeOutAffector* faff, const char* smask)
	{
		E_FADEOUT_COLOR_MASK mask = (E_FADEOUT_COLOR_MASK) 0;
		for (int i = 0; i < 4; ++i)
		{
			if (smask[i] == '\0') break;

			switch (smask[i])
			{
			case 'r': case 'R':
				mask = (E_FADEOUT_COLOR_MASK) (mask | EFCM_RED_ENABLE);
				break;
			case 'g': case 'G':
				mask = (E_FADEOUT_COLOR_MASK) (mask | EFCM_GREEN_ENABLE);
				break;
			case 'b': case 'B':
				mask = (E_FADEOUT_COLOR_MASK) (mask | EFCM_BLUE_ENABLE);
				break;
			case 'a': case 'A':
				mask = (E_FADEOUT_COLOR_MASK) (mask | EFCM_ALPHA_ENABLE);
				break;
			default:
				break;
			}
		}

		faff->setColorMask(mask);
	}}

	void luaopen_ParticleAffector( lua_State* L )
	{
		MetatableFactory<IParticleAffector>::create(L, 0);

		luaT_defRegsBgn(att_affector)
			luaT_mnamedfunc(IParticleAttractionAffector, setPoint),
			luaT_mnamedfunc(IParticleAttractionAffector, setMass),
			luaT_mnamedfunc(IParticleAttractionAffector, setRadius),
		luaT_defRegsEnd
		MetatableFactory<IParticleAttractionAffector, IParticleAffector>::create(L, att_affector);

		luaT_defRegsBgn(fo_affector)
			luaT_cnamedfunc(setTargetColor),
			luaT_cnamedfunc(setColorMask),
		luaT_defRegsEnd
		MetatableFactory<IParticleFadeOutAffector, IParticleAffector>::create(L, fo_affector);

		luaT_defRegsBgn(for_affector)
			luaT_mnamedfunc(IParticleForceAffector, setGravity),
			luaT_mnamedfunc(IParticleForceAffector, setForce),
		luaT_defRegsEnd
		MetatableFactory<IParticleForceAffector, IParticleAffector>::create(L, for_affector);

		luaT_defRegsBgn(rot_affector)
			luaT_mnnamefunc(IParticleRotationAffector, setPivotPoint, setPivot),
			luaT_mnamedfunc(IParticleRotationAffector, setSpeed),
		luaT_defRegsEnd
		MetatableFactory<IParticleRotationAffector, IParticleAffector>::create(L, rot_affector);
	}

}}
