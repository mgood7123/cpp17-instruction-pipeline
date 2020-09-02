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
    ASSERT_NE(hardware.find_wire("A"), nullptr);
}

TEST_F(Hardware_Core_With_Wires, can_connect_wire_to_another_wire) {
    hardware.connectWires("A", "B");
    Wire<int> & wire = std::any_cast<Wire<int>&>(hardware.components[0].component);
    ASSERT_EQ(wire.outputs.size(), 1);
    ASSERT_EQ(wire.data.input->size(), 0);
    ASSERT_EQ(wire.data.output->size(), 0);
}

//     hardware.connectWires("A", "C");
//     hardware.run("A", 1);
// TEST(Hardware_Core, initialization_AnyNullOpt_no_data_checking) {
//     Hardware a = AnyNullOpt;
// }
// 
// TEST(Hardware_Core, initialization_AnyNullOpt_data_checking) {
//     Hardware a = AnyNullOpt;
//     ASSERT_EQ(a.data, nullptr);
//     ASSERT_EQ(a.data_is_allocated, false);
//     ASSERT_EQ(a.isAnyNullOpt, true);
// }
// 
// TEST(Hardware_Flags_Copy, valid_copy_1) {
//     const int z = 5;
//     HardwareCustomFlags<Hardware_FLAG_COPY_ONLY> a = z;
// }
// 
// TEST(Hardware_Flags_Copy, valid_copy_2) {
//     const int z = 5;
//     const HardwareCustomFlags<Hardware_FLAG_COPY_ONLY> a = z;
//     HardwareCustomFlags<Hardware_FLAG_COPY_ONLY> b = a;
// }
// 
// TEST(Hardware_Flags_Copy, invalid_copy_1) {
//     // this will invoke the move constructor
//     ASSERT_DEATH(
//             {
//                  HardwareCustomFlags<Hardware_FLAG_COPY_ONLY> a = 5;
//              },
//             Hardware_Catch_Flag_POSIX_REGEX(Hardware_FLAG_MOVE_ONLY)
//      );
// }
// 
// TEST(Hardware_Flags_Copy, invalid_copy_2) {
//     // this will invoke the move constructor
//     ASSERT_DEATH(
//             {
//                 const int z = 5;
//                 HardwareCustomFlags<Hardware_FLAG_COPY_ONLY> a = z;
//                 HardwareCustomFlags<Hardware_FLAG_COPY_ONLY> b = a;
//             },
//             Hardware_Catch_Flag_POSIX_REGEX(Hardware_FLAG_MOVE_ONLY)
//     );
// }
// 
// TEST(Hardware_Flags_Move, valid_move) {
//     // If other is an rvalue expression,
//     // move constructor will be selected by overload resolution
//     // and called during copy-initialization.
//     // There is no such term as move-initialization.
//     // THIS only applies if a USER-DEFINED move constructor is present
//     HardwareCustomFlags<Hardware_FLAG_MOVE_ONLY> a = 5;
// }
// 
// TEST(Hardware_Flags_Pointer, valid_void_pointer_1) {
//     void * x = nullptr;
//     HardwareCustomFlags<Hardware_FLAG_ENABLE_POINTERS> a = x;
// }
// 
// TEST(Hardware_Flags_Pointer, copy_valid_void_pointer_1) {
//     void * x = nullptr;
//     const HardwareCustomFlags<Hardware_FLAG_ENABLE_POINTERS|Hardware_FLAG_COPY_ONLY> a = x;
//     HardwareCustomFlags<Hardware_FLAG_ENABLE_POINTERS|Hardware_FLAG_COPY_ONLY> b = a;
// }
// 
// TEST(Hardware_Flags_Pointer, copy_valid_void_pointer_1_fail_1) {
//     Hardware_GTEST_ASSERT_DEATH(
//             {
//                 void *x = nullptr;
//                 const HardwareCustomFlags<Hardware_FLAG_ENABLE_POINTERS> a = x;
//                 HardwareCustomFlags<Hardware_FLAG_ENABLE_POINTERS> b = a;
//             },
//             Hardware_FLAG_COPY_ONLY
//     );
// }
// 
// TEST(Hardware_Flags_Pointer, valid_void_pointer_2) {
//     void * x = new int;
//     HardwareCustomFlags<Hardware_FLAG_ENABLE_POINTERS> a = HardwareCustomFlags<Hardware_FLAG_ENABLE_POINTERS>(x, true);
// }
// 
// TEST(Hardware_Flags_Pointer, copy_valid_void_pointer_2) {
//     constexpr int flags =
//             Hardware_FLAG_ENABLE_POINTERS|
//             Hardware_FLAG_COPY_ONLY_AND_MOVE_ONLY|
//             Hardware_FLAG_ENABLE_CONVERSION_OF_ALLOCATION_COPY_TO_ALLOCATION_MOVE;
//     void * x = new int;
//     const HardwareCustomFlags<flags> a = HardwareCustomFlags<flags>(x, true);
//     HardwareCustomFlags<flags> b = a;
// }
// 
// TEST(Hardware_Flags_Pointer, copy_valid_void_pointer_2_fail_1) {
//     Hardware_GTEST_ASSERT_DEATH(
//         {
//             constexpr int flags = Hardware_FLAG_ENABLE_POINTERS;
//             void *x = new int;
//             const HardwareCustomFlags<flags> a = HardwareCustomFlags<flags>(x, true);
//             HardwareCustomFlags<flags> b = a;
//         },
//         Hardware_FLAG_COPY_ONLY
//     );
// }
// 
// TEST(Hardware_Flags_Pointer, copy_valid_void_pointer_2_fail_2) {
//     Hardware_GTEST_ASSERT_DEATH(
//             {
//                 constexpr int flags = Hardware_FLAG_ENABLE_POINTERS|Hardware_FLAG_COPY_ONLY;
//                 void *x = new int;
//                 const HardwareCustomFlags<flags> a = HardwareCustomFlags<flags>(x, true);
//                 HardwareCustomFlags<flags> b = a;
//             },
//             Hardware_FLAG_ENABLE_CONVERSION_OF_ALLOCATION_COPY_TO_ALLOCATION_MOVE
//     );
// }
// 
// TEST(Hardware_Flags_Pointer, copy_valid_void_pointer_2_fail_3) {
//     Hardware_GTEST_ASSERT_DEATH(
//             {
//                 constexpr int flags =
//                         Hardware_FLAG_ENABLE_POINTERS|
//                         Hardware_FLAG_COPY_ONLY|
//                         Hardware_FLAG_ENABLE_CONVERSION_OF_ALLOCATION_COPY_TO_ALLOCATION_MOVE;
//                 void *x = new int;
//                 const HardwareCustomFlags<flags> a = HardwareCustomFlags<flags>(x, true);
//                 HardwareCustomFlags<flags> b = a;
//             },
//             Hardware_FLAG_MOVE_ONLY
//     );
// }
// 
// TEST(Hardware_Core_Data, data_obtaining) {
//     Hardware a = 5;
//     ASSERT_EQ(a.get<int>(), 5);
//     ASSERT_EQ(a.get<int*>()[0], 5);
//     Hardware b = Hardware(new int(5), true);
//     ASSERT_EQ(b.get<int>(), 5);
//     ASSERT_EQ(b.get<int*>()[0], 5);
//     void * n = nullptr;
//     Hardware c = n;
//     ASSERT_EQ(c.get<void*>(), nullptr);
// }
// 
// TEST(Hardware_Core, equality_test) {
//     ASSERT_EQ(Hardware(5).get<int>(), Hardware(5).get<int>());
// }
