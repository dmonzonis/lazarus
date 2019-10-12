#include "catch/catch.hpp"

#include <lazarus/ECS/ECSEngine.h>
#include <lazarus/common.h>

using namespace lz;

// Global variable to test systems
int x = 0;

struct TestEvent
{
    int num;
};

struct TestComponent
{
    TestComponent(int num)
        : num(num)
    {
    }

    int num;
};

struct TestComponent2
{
    TestComponent2(int num)
    : num(num)
    {
    }

    int num;
};

class TestSystem : public Updateable, public EventListener<TestEvent>
{
public:
    virtual void update(ECSEngine& engine)
    {
        // Update global variable
        ++x;
    }

    virtual void receive(ECSEngine& engine, const TestEvent& event)
    {
        // Update global variable
        x += event.num;
    }
};

void addNumBy10(Entity* ent, TestComponent* comp)
{
    comp->num += 10;
}

TEST_CASE("add entities to the engine")
{
    ECSEngine engine;
    SECTION("add new entity")
    {
        Entity* entity = engine.add_entity();
        REQUIRE(entity != nullptr);
        Identifier id = entity->get_id();
        Entity *other = engine.add_entity();
        REQUIRE(other->get_id() == id + 1);
    }
    SECTION("add existing entity")
    {
        Entity entity;
        Identifier id = entity.get_id();
        REQUIRE_NOTHROW(engine.add_entity(entity));
        // Add new empty entity
        Entity* other = engine.add_entity();
        REQUIRE(other->get_id() == id + 1);
    }
}

TEST_CASE("get entity from identifier")
{
    ECSEngine engine;
    Entity entity;
    Identifier id = entity.get_id();
    engine.add_entity(entity);
    SECTION("get existing entity")
    {
        Entity* ent_ptr = engine.get_entity(id);
        REQUIRE(ent_ptr != nullptr);
        REQUIRE(ent_ptr->get_id() == id);
    }
    SECTION("get non-existing entity")
    {
        Entity* ent_ptr = engine.get_entity(1512);
        REQUIRE(ent_ptr == nullptr);
    }
}

TEST_CASE("entity management")
{
    ECSEngine engine;
    // Add one entity with TestComponent and TestComponent2
    Entity *entity = engine.add_entity();
    Identifier id1 = entity->get_id();
    entity->add_component<TestComponent>(0);
    entity->add_component<TestComponent2>(0);
    // Add another entity, only with TestComponent
    entity = engine.add_entity();
    Identifier id2 = entity->get_id();
    entity->add_component<TestComponent>(0);

    SECTION("entities_with_components")
    {
        // Both entities have TestComponent
        auto entities = engine.entities_with_components<TestComponent>();
        REQUIRE(entities.size() == 2);

        // Only entity 1 has TestComponent2
        entities = engine.entities_with_components<TestComponent2>();
        REQUIRE(entities.size() == 1);
        REQUIRE(entities[0]->get_id() == id1);

        // Only entity 1 has both components
        entities = engine.entities_with_components<TestComponent,
                                                 TestComponent2>();
        REQUIRE(entities.size() == 1);
        REQUIRE(entities[0]->get_id() == id1);
    }
    SECTION("apply_to_each with lambda")
    {
        // Check that components have constructed values
        TestComponent* comp = engine.get_entity(id1)->get<TestComponent>();
        REQUIRE(comp->num == 0);
        comp = engine.get_entity(id2)->get<TestComponent>();
        REQUIRE(comp->num == 0);

        // Update TestComponent on all entities that have it
        engine.apply_to_each<TestComponent>([](Entity* ent, TestComponent* comp)
        {
            comp->num++;
        });

        // Check that it was modified for both entities
        comp = engine.get_entity(id1)->get<TestComponent>();
        REQUIRE(comp->num == 1);
        comp = engine.get_entity(id2)->get<TestComponent>();
        REQUIRE(comp->num == 1);
    }
    SECTION("apply_to_each with std::function")
    {
        // Check that components have constructed values
        TestComponent* comp = engine.get_entity(id1)->get<TestComponent>();
        REQUIRE(comp->num == 0);
        comp = engine.get_entity(id2)->get<TestComponent>();
        REQUIRE(comp->num == 0);

        // Update TestComponent on all entities that have it
        engine.apply_to_each<TestComponent>(addNumBy10);

        // Check that it was modified for both entities
        comp = engine.get_entity(id1)->get<TestComponent>();
        REQUIRE(comp->num == 10);
        comp = engine.get_entity(id2)->get<TestComponent>();
        REQUIRE(comp->num == 10);
    }
}

TEST_CASE("event management")
{
    ECSEngine engine;
    TestEvent event{10};
    x = 0;
    SECTION("event listener subscription/unsubscription")
    {
        // System is not subscribed yet
        REQUIRE_NOTHROW(engine.delete_system<TestSystem>());
        // Add system
        REQUIRE_NOTHROW(engine.add_system<TestSystem, TestEvent>());
        // Try to add another similar system
        REQUIRE_NOTHROW(engine.add_system<TestSystem, TestEvent>());
        // Unsubscribe original system
        REQUIRE_NOTHROW(engine.delete_system<TestSystem>());
    }
    SECTION("emitting and receiving events from the engine without subscribers")
    {
        // Emit without any subscribers
        engine.emit(event);
        REQUIRE(x == 0);
    }
    SECTION("emitting and receiving events from the engine with subscribers")
    {
        // Emit with subscribers
        REQUIRE_NOTHROW(engine.add_system<TestSystem, TestEvent>());
        engine.emit(event);
        REQUIRE(x == 10);
        // Adding more of the same system to the same event type will not create a new system
        REQUIRE_NOTHROW(engine.add_system<TestSystem, TestEvent>());
        REQUIRE_NOTHROW(engine.add_system<TestSystem, TestEvent>());
        engine.emit(event);
        REQUIRE(x == 20);
    }
}

TEST_CASE("updateable management")
{
    ECSEngine engine;
    TestSystem system;
    x = 0;
    SECTION("update without subscribers")
    {
        engine.update();
        REQUIRE(x == 0);
    }
    SECTION("update with subscribers")
    {
        engine.register_updateable(&system);
        engine.update();
        REQUIRE(x == 1);
    }
}

TEST_CASE("garbage collector")
{
    ECSEngine engine;
    Entity *entity = engine.add_entity();
    Identifier id = entity->get_id();
    REQUIRE_FALSE(entity->is_deleted());
    REQUIRE(engine.get_entity(id) != nullptr);
    
    // Mark for deletion
    entity->mark_for_deletion();
    REQUIRE(engine.get_entity(id) != nullptr);
    REQUIRE(entity->is_deleted());
    
    // Garbage collect
    engine.update();
    REQUIRE(engine.get_entity(id) == nullptr);
}
