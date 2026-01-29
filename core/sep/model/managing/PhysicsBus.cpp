//
// Created by down1 on 26.01.2026.
//

#include "PhysicsBus.h"

#include "Engine.hpp"


glm::mat4 PhysicsBus::JoltToGlm(const JPH::RMat44 &joltMatrix) {
	glm::mat4 glmMatrix;
	joltMatrix.StoreFloat4x4(reinterpret_cast<JPH::Float4*>(&glmMatrix));
	return glmMatrix;
}
static void JoltTraceImpl(const char* inFMT, ...) {
	va_list list;
	va_start(list, inFMT);
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), inFMT, list);
	va_end(list);
	std::cout << "[Jolt Trace]: " << buffer << std::endl;
}

// Функцию для ассертов можно оставить лямбдой (там нет "...", поэтому Clang не ноет),
// но для красоты можно тоже вынести
static bool JoltAssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint inLine) {
	std::cerr << "[Jolt Assert]: " << inExpression << " | Msg: " << (inMessage ? inMessage : "")
			  << " | File: " << inFile << ":" << inLine << std::endl;
	return true; // true = прервать выполнение
}

#pragma region Main
void PhysicsBus::initializeJPH() {
	JPH::RegisterDefaultAllocator();

	// Install trace and assert callbacks
	JPH::Trace = JoltTraceImpl;
	JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = JoltAssertFailedImpl;)

	// Create a factory, this class is responsible for creating instances of classes based on their name or hash and is mainly used for deserialization of saved data.
	// It is not directly used in this example but still required.
	JPH::Factory::sInstance = new JPH::Factory();

	JPH::RegisterTypes();

	temp_allocator = new JPH::TempAllocatorImpl (ALLOCATOR_SIZE);
	job_system = new JPH::JobSystemThreadPool(MAX_JOBS, MAX_BARRIERS, std::thread::hardware_concurrency() - 1); // Init multithreaded jobs system

	physics_system = new JPH::PhysicsSystem();

	// Physics parameters
	constexpr uint cMaxBodies = 1024;
	constexpr uint cMaxBodyPairs = 2048;
	constexpr uint cMaxContactConstraints = 1024;
	constexpr uint cNumBodyMutexes = 0;

	// Create mapping table from object layer to broadphase layer
	// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
	// Also have a look at BroadPhaseLayerInterfaceTable or BroadPhaseLayerInterfaceMask for a simpler interface.
	bp_interface = new BPLayerInterfaceImpl();

	// Create class that filters object vs broadphase layers
	// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
	// Also have a look at ObjectVsBroadPhaseLayerFilterTable or ObjectVsBroadPhaseLayerFilterMask for a simpler interface.
	obj_bp_filter = new ObjectVsBroadPhaseLayerFilterImpl();

	// Create class that filters object vs object layers
	// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
	// Also have a look at ObjectLayerPairFilterTable or ObjectLayerPairFilterMask for a simpler interface.
	obj_obj_filter = new ObjectLayerPairFilterImpl();

	// Now we can create the actual physics system.
	physics_system->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, *bp_interface, *obj_bp_filter, *obj_obj_filter);

	// A body activation listener gets notified when bodies activate and go to sleep
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	body_activation_listener = new MyBodyActivationListener();
	physics_system->SetBodyActivationListener(body_activation_listener);

	// A contact listener gets notified when bodies (are about to) collide, and when they separate again.
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	contact_listener = new MyContactListener();
	physics_system->SetContactListener(contact_listener);

	// The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
	// variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)

	physics_system->OptimizeBroadPhase();
	physics_system->SetGravity(JPH::Vec3(0.0f, 0.0f, -9.8f));
}

void PhysicsBus::iterateJPH(Engine& engine) const {
	// ШАГ 1: Обновляем физический мир (ОДИН РАЗ за вызов функции)
	// 1 — это количество подшагов (collision steps)
	physics_system->Update(cDeltaTime, 1, temp_allocator, job_system);

	// ШАГ 2: Получаем активные тела для синхронизации графики
	JPH::BodyIDVector activeBodies;
	physics_system->GetActiveBodies(JPH::EBodyType::RigidBody, activeBodies);

	const JPH::BodyInterface &body_interface = physics_system->GetBodyInterface();

	for (JPH::BodyID id : activeBodies) {
		// Используем GetWorldTransform без лока, так как мы сейчас в одном потоке
		// и Update уже завершен. Это быстрее и безопаснее.
		JPH::RMat44 transform = body_interface.GetWorldTransform(id);

		// Проверяем, есть ли тело в нашем маппинге
		if (engine.mem.bodyID.contains(id)) {
			auto& data = engine.mem.bodyID[id];
			engine.updateSingleModel(data.first, data.second, JoltToGlm(transform));
		}
	}
}

void PhysicsBus::shutdownJPH() const {
	JPH::BodyInterface &body_interface = physics_system->GetBodyInterface();

	JPH::BodyIDVector bodies;
	physics_system->GetBodies(bodies);

	for (JPH::BodyID id : bodies) {
		body_interface.RemoveBody(id);
		body_interface.DestroyBody(id);
	}

	JPH::UnregisterTypes();

	delete JPH::Factory::sInstance;
	JPH::Factory::sInstance = nullptr;
}
#pragma endregion

#pragma region Body
JPH::BodyID PhysicsBus::createBody(JPH::BodyCreationSettings body_settings) const {
	JPH::BodyInterface &body_interface = physics_system->GetBodyInterface();

	body_settings.mEnhancedInternalEdgeRemoval = true;
	body_settings.mMotionQuality = JPH::EMotionQuality::LinearCast;
	body_settings.mUserData = 1;

	return body_interface.CreateAndAddBody(body_settings, JPH::EActivation::Activate);
}
#pragma endregion