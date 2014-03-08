#include "IrrlichtComponentSystemFactory.h"
#include "IrrlichtComponentSystem.h"
#include "Engine/GameScene.h"
#include <irrlicht/IrrlichtDevice.h>
#include <irrlicht/ISceneManager.h>

using namespace xihad::ngn;
using namespace irr;
using namespace std;
namespace xihad { namespace render3d
{
	IrrlichtComponentSystemFactory::IrrlichtComponentSystemFactory(IrrlichtDevice* device) :
		MultiComponentSystemFactory("Render"), mDevice(device)
	{
		xassert(mDevice);
		init();
	}

	void IrrlichtComponentSystemFactory::init()
	{
		registerDerivedType("Mesh"			);
		registerDerivedType("SkyDome"		);
		registerDerivedType("Billboard"		);
		registerDerivedType("Camera"		);
		registerDerivedType("Light"			);
		registerDerivedType("Terrian"		);
		registerDerivedType("Text"			);
		registerDerivedType("AnimatedMesh"	);
		registerDerivedType("ParticleSystem");
	}

	irr::IrrlichtDevice* IrrlichtComponentSystemFactory::getDevice()
	{
		return mDevice.get();
	}

	ngn::ComponentSystem* IrrlichtComponentSystemFactory::createMainSystem( ngn::GameScene* scene )
	{
		scene::ISceneManager* irrScene = mDevice->getSceneManager();

		// Don't add ref in the constructor, because create function
		// have already added a ref
		irr_ptr<scene::ISceneManager> newScene(irrScene->createNewSceneManager(false), false);

		float ambient = .3f;
		newScene->setAmbientLight(video::SColorf(ambient, ambient, ambient));
		newScene->setShadowColor(video::SColor(60,0,0,0));
		
		return new IrrlichtComponentSystem(mDevice.get(), newScene.get(), *this, mCachedClips);
	}

}}