//
// Created by smallville7123 on 9/08/20.
//

#include <gtest/gtest.h>

#include <Hardware.hpp>

class Hardware_Core : public ::testing::Test {
 protected:
  // void SetUp() override {}

  // void TearDown() override {}

  Hardware<int> hardware;
};

class Hardware_Core_With_Wires : public Hardware_Core {
 protected:
  void SetUp() override {
      hardware.addWire("A");
      hardware.addWire("B");
      hardware.addWire("C");
  }

  // void TearDown() override {}

  Hardware<int> hardware;
};

TEST_F(Hardware_Core, initialization_contains_expected_values) {
    ASSERT_NE(hardware.logger, nullptr);
    ASSERT_EQ(hardware.components.size(), 0);
}

TEST_F(Hardware_Core, can_add_wire) {
    hardware.addWire("A");
    ASSERT_EQ(hardware.components.size(), 1);
}

TEST_F(Hardware_Core, added_wire_has_expected_values) {
    hardware.addWire("A");
    ASSERT_EQ(hardware.components.size(), 1);
    ASSERT_EQ(hardware.components[0].type, ComponentTypes::Wire);
    ASSERT_STREQ(hardware.components[0].id.c_str(), "A");
    Wire<int> & wire = std::any_cast<Wire<int>&>(hardware.components[0].component);
    ASSERT_EQ(wire.outputs.size(), 0);
    ASSERT_EQ(wire.data.input->size(), 0);
    ASSERT_EQ(wire.data.output->size(), 0);
}

TEST_F(Hardware_Core, can_add_multiple_wires) {
    hardware.addWire("A");
    hardware.addWire("B");
    ASSERT_EQ(hardware.components.size(), 2);
}

TEST_F(Hardware_Core, added_wires_has_expected_values) {
    hardware.addWire("A");
    hardware.addWire("B");
    ASSERT_EQ(hardware.components.size(), 2);
    
    ASSERT_EQ(hardware.components[0].type, ComponentTypes::Wire);
    ASSERT_STREQ(hardware.components[0].id.c_str(), "A");
    Wire<int> & wire1 = std::any_cast<Wire<int>&>(hardware.components[0].component);
    ASSERT_EQ(wire1.outputs.size(), 0);
    ASSERT_EQ(wire1.data.input->size(), 0);
    ASSERT_EQ(wire1.data.output->size(), 0);
    
    ASSERT_EQ(hardware.components[1].type, ComponentTypes::Wire);
    ASSERT_STREQ(hardware.components[1].id.c_str(), "B");
    Wire<int> & wire2 = std::any_cast<Wire<int>&>(hardware.components[1].component);
    ASSERT_EQ(wire2.outputs.size(), 0);
    ASSERT_EQ(wire2.data.input->size(), 0);
    ASSERT_EQ(wire2.data.output->size(), 0);
}

TEST_F(Hardware_Core_With_Wires, can_find_component) {
    ASSERT_EQ(hardware.find_component("A").has_value(), true);
}

TEST_F(Hardware_Core_With_Wires, cannot_find_component) {
    ASSERT_EQ(hardware.find_component("P").has_value(), false);
}

TEST_F(Hardware_Core_With_Wires, can_get_component) {
    hardware.component_get<Wire>("A");
}

TEST_F(Hardware_Core_With_Wires, cannot_get_component) {
    ASSERT_THROW(
        {
            hardware.component_get<Wire>("P");
        },
        std::bad_optional_access
    );
}

TEST_F(Hardware_Core_With_Wires, can_find_wire) {
    auto wire = hardware.component_cast<Wire>(hardware.find_component("A"));
    ASSERT_EQ(wire.has_value(), true);
    ASSERT_EQ(wire.value().get().outputs.size(), 0);
    ASSERT_EQ(wire.value().get().data.input->size(), 0);
    ASSERT_EQ(wire.value().get().data.output->size(), 0);
}

TEST_F(Hardware_Core_With_Wires, wire_can_store_data) {
    Wire<int> & wire = hardware.component_get<Wire>("A");
    wire.push(std::move(5));
    ASSERT_EQ(wire.data.input->size(), 0);
    ASSERT_EQ(wire.data.output->size(), 1);
}

TEST_F(Hardware_Core_With_Wires, can_connect_wire_to_another_wire) {
    hardware.connectWires("A", "B");
    auto wire1 = hardware.component_get<Wire>("A");
    ASSERT_EQ(wire1.outputs.size(), 1);
    ASSERT_EQ(wire1.data.input->size(), 0);
    ASSERT_EQ(wire1.data.output->size(), 0);
    auto wire2 = hardware.component_get<Wire>("B");
    ASSERT_EQ(wire2.outputs.size(), 0);
    ASSERT_EQ(wire2.data.input->size(), 0);
    ASSERT_EQ(wire2.data.output->size(), 0);
}

TEST_F(Hardware_Core_With_Wires, can_send_data_from_one_wire_to_another_wire) {
    hardware.connectWires("A", "B");
    hardware.run("A", 1);
    auto wire1 = hardware.component_get<Wire>("A");
    ASSERT_EQ(wire1.outputs.size(), 1);
    ASSERT_EQ(wire1.data.input->size(), 0);
    ASSERT_EQ(wire1.data.output->size(), 0);
    auto wire2 = hardware.component_get<Wire>("B");
    ASSERT_EQ(wire2.outputs.size(), 0);
    ASSERT_EQ(wire2.data.input->size(), 0);
    ASSERT_EQ(wire2.data.output->size(), 1);
    int && output = std::move(wire2.pull());
    ASSERT_EQ(wire2.data.output->size(), 0);
    ASSERT_EQ(output, 1);
}

TEST_F(Hardware_Core_With_Wires, can_connect_wire_to_multiple_wires) {
    hardware.connectWires("A", "B");
    hardware.connectWires("A", "C");
    auto wire1 = hardware.component_get<Wire>("A");
    ASSERT_EQ(wire1.outputs.size(), 2);
    ASSERT_EQ(wire1.data.input->size(), 0);
    ASSERT_EQ(wire1.data.output->size(), 0);
    auto wire2 = hardware.component_get<Wire>("B");
    ASSERT_EQ(wire2.outputs.size(), 0);
    ASSERT_EQ(wire2.data.input->size(), 0);
    ASSERT_EQ(wire2.data.output->size(), 0);
    auto wire3 = hardware.component_get<Wire>("C");
    ASSERT_EQ(wire2.outputs.size(), 0);
    ASSERT_EQ(wire2.data.input->size(), 0);
    ASSERT_EQ(wire2.data.output->size(), 0);
}

TEST_F(Hardware_Core_With_Wires, can_send_data_from_one_wire_to_multiple_wires) {
    hardware.connectWires("A", "B");
    hardware.connectWires("A", "C");
    hardware.run("A", 1);
    
    auto wire1 = hardware.component_get<Wire>("B");
    int && output1 = std::move(wire1.pull());
    ASSERT_EQ(wire1.data.output->size(), 0);
    ASSERT_EQ(output1, 1);

    auto wire2 = hardware.component_get<Wire>("C");
    int && output2 = std::move(wire2.pull());
    ASSERT_EQ(wire2.data.output->size(), 0);
    ASSERT_EQ(output2, 1);
}

TEST_F(Hardware_Core_With_Wires, invalid_connection) {
    ASSERT_DEATH(
            {
                hardware.connectWires("A", "F");
            },
            ".*wire does not exist: F"
     );
}
