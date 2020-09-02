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
    Wire<int> & wire = std::any_cast<Wire<int>&>(hardware.components[0].component);
    ASSERT_STREQ(wire.id.c_str(), "A");
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
    Wire<int> & wire1 = std::any_cast<Wire<int>&>(hardware.components[0].component);
    ASSERT_STREQ(wire1.id.c_str(), "A");
    ASSERT_EQ(wire1.outputs.size(), 0);
    ASSERT_EQ(wire1.data.input->size(), 0);
    ASSERT_EQ(wire1.data.output->size(), 0);
    
    ASSERT_EQ(hardware.components[1].type, ComponentTypes::Wire);
    Wire<int> & wire2 = std::any_cast<Wire<int>&>(hardware.components[1].component);
    ASSERT_STREQ(wire2.id.c_str(), "B");
    ASSERT_EQ(wire2.outputs.size(), 0);
    ASSERT_EQ(wire2.data.input->size(), 0);
    ASSERT_EQ(wire2.data.output->size(), 0);
}

TEST_F(Hardware_Core_With_Wires, can_find_wire) {
    Wire<int> * wire = hardware.find_wire("A");
    ASSERT_NE(wire, nullptr);
    ASSERT_STREQ(wire->id.c_str(), "A");
    ASSERT_EQ(wire->outputs.size(), 0);
    ASSERT_EQ(wire->data.input->size(), 0);
    ASSERT_EQ(wire->data.output->size(), 0);
}

TEST_F(Hardware_Core_With_Wires, wire_can_store_data) {
    Wire<int> * wire = hardware.find_wire("A");
    wire->push(std::move(5));
    ASSERT_EQ(wire->data.input->size(), 0);
    ASSERT_EQ(wire->data.output->size(), 1);
}

TEST_F(Hardware_Core_With_Wires, can_connect_wire_to_another_wire) {
    hardware.connectWires("A", "B");
    Wire<int> * wire1 = hardware.find_wire("A");
    ASSERT_EQ(wire1->outputs.size(), 1);
    ASSERT_EQ(wire1->data.input->size(), 0);
    ASSERT_EQ(wire1->data.output->size(), 0);
    Wire<int> * wire2 = hardware.find_wire("B");
    ASSERT_EQ(wire2->outputs.size(), 0);
    ASSERT_EQ(wire2->data.input->size(), 0);
    ASSERT_EQ(wire2->data.output->size(), 0);
}

TEST_F(Hardware_Core_With_Wires, can_send_data_from_one_wire_to_another_wire) {
    hardware.connectWires("A", "B");
    hardware.run("A", 1);
    Wire<int> * wire1 = hardware.find_wire("A");
    ASSERT_EQ(wire1->outputs.size(), 1);
    ASSERT_EQ(wire1->data.input->size(), 0);
    ASSERT_EQ(wire1->data.output->size(), 0);
    Wire<int> * wire2 = hardware.find_wire("B");
    ASSERT_EQ(wire2->outputs.size(), 0);
    ASSERT_EQ(wire2->data.input->size(), 0);
    ASSERT_EQ(wire2->data.output->size(), 1);
    int && output = std::move(wire2->pull());
    ASSERT_EQ(wire2->data.output->size(), 0);
    ASSERT_EQ(output, 1);
}

TEST_F(Hardware_Core_With_Wires, can_connect_wire_to_multiple_wires) {
    hardware.connectWires("A", "B");
    hardware.connectWires("A", "C");
    Wire<int> * wire1 = hardware.find_wire("A");
    ASSERT_EQ(wire1->outputs.size(), 2);
    ASSERT_EQ(wire1->data.input->size(), 0);
    ASSERT_EQ(wire1->data.output->size(), 0);
    Wire<int> * wire2 = hardware.find_wire("B");
    ASSERT_EQ(wire2->outputs.size(), 0);
    ASSERT_EQ(wire2->data.input->size(), 0);
    ASSERT_EQ(wire2->data.output->size(), 0);
    Wire<int> * wire3 = hardware.find_wire("C");
    ASSERT_EQ(wire2->outputs.size(), 0);
    ASSERT_EQ(wire2->data.input->size(), 0);
    ASSERT_EQ(wire2->data.output->size(), 0);
}

TEST_F(Hardware_Core_With_Wires, can_send_data_from_one_wire_to_multiple_wires) {
    hardware.connectWires("A", "B");
    hardware.connectWires("A", "C");
    hardware.run("A", 1);
    
    Wire<int> * wire1 = hardware.find_wire("B");
    int && output1 = std::move(wire1->pull());
    ASSERT_EQ(wire1->data.output->size(), 0);
    ASSERT_EQ(output1, 1);

    Wire<int> * wire2 = hardware.find_wire("C");
    int && output2 = std::move(wire2->pull());
    ASSERT_EQ(wire2->data.output->size(), 0);
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
