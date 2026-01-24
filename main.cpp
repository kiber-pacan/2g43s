#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>

#include <cmath>
#include <thread>

#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#include <imgui_impl_sdl3.h>
#include <glm/glm.hpp>
#include <omp.h>
#include <basisu_transcoder.h>


#include "KeyListener.hpp"
#include "MouseListener.hpp"
#include "Random.hpp"
#include "Engine.hpp"
#include "Logger.hpp"
#include "Tools.hpp"

#include <Jolt/Jolt.h>

// Jolt includes
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

#include "AtomicCounterObject.hpp"
#include "AtomicCounterObject.hpp"


Logger LOGGER = Logger("main.cpp");
#pragma region Jolt
constexpr uint ALLOCATOR_SIZE = 10 * 1024 * 1024; // 10 MB
constexpr uint MAX_JOBS = 4096;
constexpr uint MAX_BARRIERS = 16;

std::vector<JPH::BodyID> physicsBodies{};

#pragma region Generic
namespace Layers {
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
    static constexpr JPH::ObjectLayer MOVING = 1;
    static constexpr JPH::uint NUM_LAYERS = 2;
}

class ObjectLayerPairFilterImpl final : public JPH::ObjectLayerPairFilter {
public:
    [[nodiscard]] bool ShouldCollide(const JPH::ObjectLayer firstObject, const JPH::ObjectLayer secondObject) const override {
        switch (firstObject) {
            case Layers::NON_MOVING:
                // Nonmoving objects collide only with moving ones
                return secondObject == Layers::MOVING;
            case Layers::MOVING:
                // Moving objects collides with everything
                return true;
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

namespace BroadPhaseLayers {
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr uint NUM_LAYERS(2);
}

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
public:
    BPLayerInterfaceImpl() {
        // Create a mapping table from object to broad phase layer
        mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
    }

    [[nodiscard]] uint GetNumBroadPhaseLayers() const override {
        return BroadPhaseLayers::NUM_LAYERS;
    }

    [[nodiscard]] JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
        JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
        return mObjectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    [[nodiscard]] const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override {
        switch (static_cast<JPH::BroadPhaseLayer::Type>(inLayer)) {
            case static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayers::NON_MOVING):
                return "NON_MOVING";
            case static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayers::MOVING):
                return "MOVING";
            default:
                JPH_ASSERT(false); return "INVALID";
        }
    }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl final : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    [[nodiscard]] bool ShouldCollide(JPH::ObjectLayer firstLayer, JPH::BroadPhaseLayer secondLayer) const override {
        switch (firstLayer) {
            case Layers::NON_MOVING:
                return secondLayer == BroadPhaseLayers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

class MyContactListener final : public JPH::ContactListener {
    Logger LOGGER = Logger("ContactListener");
public:
    // See: ContactListener
    JPH::ValidateResult	OnContactValidate(const JPH::Body &inBody1, const JPH::Body &inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult &inCollisionResult) override {
        //LOGGER.info("Contact validate callback");

        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override {
        LOGGER.info("A contact was added");
    }

    void OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override {
        //LOGGER.info("A contact was persisted");
    }

    void OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) override {
        LOGGER.info("A contact was removed");
    }
};

// An example activation listener
class MyBodyActivationListener final : public JPH::BodyActivationListener {
    Logger LOGGER = Logger("BodyActivationListener");
public:
    void OnBodyActivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData) override {
        LOGGER.info("A body got activated");
    }

    void OnBodyDeactivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData) override {
        LOGGER.info("A body went to sleep");
    }
};

#pragma endregion
#pragma endregion

struct AppContext {
    Engine engine;
    KeyListener* kL;
    MouseListener* mL;

    JPH::TempAllocatorImpl* temp_allocator{};
    JPH::JobSystemThreadPool* job_system{};
    JPH::PhysicsSystem* physics_system{};

    BPLayerInterfaceImpl* bp_interface{};
    ObjectVsBroadPhaseLayerFilterImpl* obj_bp_filter{};
    ObjectLayerPairFilterImpl* obj_obj_filter{};

    // Листенеры тоже должны жить долго!
    MyBodyActivationListener* body_activation_listener{};
    MyContactListener* contact_listener{};

    SDL_Thread* tickThread = nullptr;
};

#pragma region JoltMain
void initializeJPH(AppContext* app) {
    JPH::RegisterDefaultAllocator();

	// Install trace and assert callbacks
	//JPH::Trace = JPH::TraceImpl;
	//JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)

	// Create a factory, this class is responsible for creating instances of classes based on their name or hash and is mainly used for deserialization of saved data.
	// It is not directly used in this example but still required.
	JPH::Factory::sInstance = new JPH::Factory();

	JPH::RegisterTypes();

    app->temp_allocator = new JPH::TempAllocatorImpl (ALLOCATOR_SIZE);
	app->job_system = new JPH::JobSystemThreadPool(MAX_JOBS, MAX_BARRIERS, std::thread::hardware_concurrency() - 1); // Init multithreaded jobs system

    app->physics_system = new JPH::PhysicsSystem();

    // Physics parameters
    constexpr uint cMaxBodies = 2048;
    constexpr uint cMaxBodyPairs = 2048;
    constexpr uint cMaxContactConstraints = 1024;
    constexpr uint cNumBodyMutexes = 0;

	// Create mapping table from object layer to broadphase layer
	// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
	// Also have a look at BroadPhaseLayerInterfaceTable or BroadPhaseLayerInterfaceMask for a simpler interface.
	app->bp_interface = new BPLayerInterfaceImpl();

	// Create class that filters object vs broadphase layers
	// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
	// Also have a look at ObjectVsBroadPhaseLayerFilterTable or ObjectVsBroadPhaseLayerFilterMask for a simpler interface.
	app->obj_bp_filter = new ObjectVsBroadPhaseLayerFilterImpl();

	// Create class that filters object vs object layers
	// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
	// Also have a look at ObjectLayerPairFilterTable or ObjectLayerPairFilterMask for a simpler interface.
	app->obj_obj_filter = new ObjectLayerPairFilterImpl();

	// Now we can create the actual physics system.
	app->physics_system->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, *app->bp_interface, *app->obj_bp_filter, *app->obj_obj_filter);

	// A body activation listener gets notified when bodies activate and go to sleep
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	app->body_activation_listener = new MyBodyActivationListener();
	app->physics_system->SetBodyActivationListener(app->body_activation_listener);

	// A contact listener gets notified when bodies (are about to) collide, and when they separate again.
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	app->contact_listener = new MyContactListener();
	app->physics_system->SetContactListener(app->contact_listener);

	// The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
	// variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)
	JPH::BodyInterface &body_interface = app->physics_system->GetBodyInterface();

    #pragma region floor
	// Next we can create a rigid body to serve as the floor, we make a large box
	// Create the settings for the collision volume (the shape).
	// Note that for simple shapes (like boxes) you can also directly construct a BoxShape.
	//JPH::BoxShapeSettings floor_shape_settings(JPH::Vec3(50.0f, 50.0f, 1.0f));
	//floor_shape_settings.SetEmbedded(); // A ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.
	//JPH::ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
	//JPH::ShapeRefC floor_shape = floor_shape_result.Get(); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()
    auto floor_shape = app->engine.mdlBus.groups_map["land2_opt.glb"].model.get()->createJoltMesh();
	JPH::BodyCreationSettings floor_settings(floor_shape, JPH::RVec3(0.0, 0.0, -16.0), JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::NON_MOVING);
	// Create the actual rigid body
	JPH::Body *floor = body_interface.CreateBody(floor_settings); // Note that if we run out of bodies this can return nullptr

	// Add it to the world
	body_interface.AddBody(floor->GetID(), JPH::EActivation::DontActivate);
    #pragma endregion

    #pragma region sphere
	// Now create a dynamic body to bounce on the floor
	// Note that this uses the shorthand version of creating and adding a body to the world
	JPH::BodyCreationSettings sphere_settings(new JPH::SphereShape(5.0f), JPH::RVec3(0.0, 0.0, 64.0), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING);

    sphere_settings.mEnhancedInternalEdgeRemoval = true;
    sphere_settings.mMotionQuality = JPH::EMotionQuality::LinearCast;
    sphere_settings.mUserData = 1;

    JPH::BodyID sphere_id = body_interface.CreateAndAddBody(sphere_settings, JPH::EActivation::Activate);

	// Now you can interact with the dynamic body, in this case we're going to give it a velocity.
	// (note that if we had used CreateBody then we could have set the velocity straight on the body before adding it to the physics system)
	//body_interface.SetLinearVelocity(sphere_id, JPH::Vec3(4.0f, 12.0f, 1.0f));
    #pragma endregion

	app->physics_system->OptimizeBroadPhase();
    app->physics_system->SetGravity(JPH::Vec3(0.0f, 0.0f, -9.8f));
}
constexpr uint TICK_RATE = 256;
constexpr float cDeltaTime = 1.0f / TICK_RATE;

glm::mat4 JoltToGlm(const JPH::RMat44 &joltMatrix) {
    glm::mat4 glmMatrix;
    joltMatrix.StoreFloat4x4(reinterpret_cast<JPH::Float4*>(&glmMatrix));
    return glmMatrix;
}

void iterateJPH(AppContext* app) {
    JPH::BodyIDVector activeBodies;
    app->physics_system->GetActiveBodies(JPH::EBodyType::RigidBody, activeBodies);

    for (JPH::BodyID id : activeBodies) {
        const JPH::BodyLockRead lock(app->physics_system->GetBodyLockInterface(), id);
        if (lock.Succeeded()) {
            const JPH::Body &body = lock.GetBody();
            //app->engine.updateSingleModel(0, JoltToGlm(body.GetWorldTransform()));
        }
    }
}

void shutdownJPH(const AppContext* app) {
    JPH::BodyInterface &body_interface = app->physics_system->GetBodyInterface();

    // Destroy the sphere. After this the sphere ID is no longer valid.
    JPH::BodyIDVector bodies;
    app->physics_system->GetBodies(bodies);

    for (JPH::BodyID id : bodies) {
        body_interface.RemoveBody(id);
        body_interface.DestroyBody(id);
    }

    // Unregisters all types with the factory and cleans up the default material
    JPH::UnregisterTypes();

    // Destroy the factory
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
}
#pragma endregion

int TickThread(void* ptr) {
    auto* app = static_cast<AppContext*>(ptr);
    constexpr double dt = 1.0 / TICK_RATE;

    while (!app->engine.quit) {
        const uint64_t start = SDL_GetTicksNS();

        const int cCollisionSteps = 1;
        app->physics_system->Update(dt, cCollisionSteps, app->temp_allocator, app->job_system);

        iterateJPH(app);

        const uint64_t elapsed = SDL_GetTicksNS() - start;
        double sleepTime = dt - static_cast<double>(elapsed) / 1'000'000'000.0;
        if (sleepTime > 0) SDL_DelayNS(static_cast<Uint64>(sleepTime * 1'000'000'000.0));
    }
    return 0;
}

#pragma region SDL3
SDL_AppResult SDL_Fail() {
    SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError());
    return SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    #if defined(__linux__)
        setenv("OMP_PROC_BIND", "true", 1);
        setenv("OMP_PLACES", "cores", 1);
    #endif

    basist::basisu_transcoder_init();

    // Init the library, here we make a window so we only need the Video capabilities.
    if (!SDL_Init(SDL_INIT_VIDEO)){
        return SDL_Fail();
    }

    // Create a window
    SDL_Window* window = SDL_CreateWindow("Window", WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);
    if (!window){
        return SDL_Fail();
    }


    // Print some information about the window
    SDL_ShowWindow(window);
    {
        int width, height, bbwidth, bbheight;
        SDL_GetWindowSize(window, &width, &height);
        SDL_GetWindowSizeInPixels(window, &bbwidth, &bbheight);
        LOGGER.info("Window size: ${} x ${}", width, height);
        LOGGER.info("Backbuffer size: ${} x ${}", bbwidth, bbheight);
        if (width != bbwidth){
            LOGGER.info("This is a highdpi environment.");
        }
        LOGGER.info("Random number: ${} (between 1 and 100)", Random::randomNum<uint32_t>(1,100));
    }

    // Set up Vulkan engine
    Engine vulkan_engine;
    vulkan_engine.init(window);


    // set up the application data
    *appstate = new AppContext{
        vulkan_engine,
        new KeyListener{},
        new MouseListener{},
    };

    auto* app = static_cast<AppContext*>(*appstate);
    initializeJPH(app);
    app->tickThread = SDL_CreateThread(TickThread, "TickThread", app);

    if (!app->tickThread) {
        LOGGER.error("Failed to INITIALIZE tick thread!");
        return SDL_APP_FAILURE;
    }

    LOGGER.info("Session ID: ${}", vulkan_engine.sid);
    LOGGER.success("Application STARTED successfully!");

    SDL_GetRelativeMouseState(&vulkan_engine.mousePosition.x, &vulkan_engine.mousePosition.y);
    SDL_GetRelativeMouseState(&vulkan_engine.mousePointerPosition.x, &vulkan_engine.mousePointerPosition.y);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *e) {
    auto* app = static_cast<AppContext *>(appstate);

    // Close on hitting button
    if (e->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    // Close on ESC
    if(app->engine.quit) {
        return SDL_APP_SUCCESS;
    }

    if (e->type == SDL_EVENT_WINDOW_RESIZED) {
        int width, height;
        SDL_GetWindowSizeInPixels(app->engine.window, &width, &height);
        app->engine.framebufferResized = true;
    }

    app->kL->listen(e);

    if (SDL_GetWindowRelativeMouseMode(app->engine.window)) {
        app->mL->listen(app->engine.camera);
    } else {
        ImGui_ImplSDL3_ProcessEvent(e);
    }

    return SDL_APP_CONTINUE;
}


static std::chrono::duration<double> duration;

SDL_AppResult SDL_AppIterate(void *appstate) {
    const auto start = std::chrono::high_resolution_clock::now();

    auto* app = static_cast<AppContext *>(appstate);
    const auto& sleepTime = app->engine.sleepTimeTotalSeconds;

    app->engine.delta.calculateDelta();
    app->kL->iterateKeys(app->engine);

    app->engine.drawFrame();
    vkDeviceWaitIdle(app->engine.device);

    duration = std::chrono::high_resolution_clock::now() - start;

    #pragma region fpsLimit
    if (duration.count() > sleepTime) return SDL_APP_CONTINUE;

    std::this_thread::sleep_for(std::chrono::duration<double>(sleepTime - duration.count() - sleepTime / 32)); // Rough sleep

    // Precise sleep loop
    while (duration.count() < sleepTime) {
        duration = std::chrono::high_resolution_clock::now() - start;
    }
    #pragma endregion

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    // Cleanup
    if (auto* app = static_cast<AppContext *>(appstate)) {
        app->engine.cleanup();
        SDL_DestroyWindow(app->engine.window);
        shutdownJPH(app);
        delete app;
    }

    SDL_Quit();
    LOGGER.success("Application quit successfully!");
}
#pragma endregion