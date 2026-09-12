#include "uec.h"
#include <cstdint>
#include <memory>
#include <gtest/gtest.h>

#include "eventlist.h"
#include "route.h"
#include "uec_mp.h"

const linkspeed_bps LINKSPEED   = speedFromGbps(100);
constexpr uint32_t  NO_OF_PORTS = 1;
constexpr uint16_t  NO_OF_PATHS = 16;

// Start times are specified in picoseconds.  The second one is deliberately
// larger than 2^32 picoseconds.
constexpr simtime_picosec FIRST_START_PICOSECONDS  = 40000000;    // 40us
constexpr simtime_picosec SECOND_START_PICOSECONDS = 5000000000;  // 5ms

class UecSrcTest : public ::testing::Test {
   protected:
    std::unique_ptr<EventList> eventlist_;
    std::unique_ptr<UecNIC>    nic_;
    Route                      routeout_, routeback_;

    virtual void SetUp() {
        eventlist_ = std::make_unique<EventList>();
        eventlist_->setEndtime(timeFromSec(1));
        nic_ = std::make_unique<UecNIC>(0, *eventlist_, LINKSPEED, NO_OF_PORTS);
    }

    std::unique_ptr<UecSrc> makeSrc() {
        return std::make_unique<UecSrc>(nullptr, *eventlist_,
                                        std::make_unique<UecMpBitmap>(NO_OF_PATHS, false),
                                        *nic_, NO_OF_PORTS);
    }

    std::unique_ptr<UecSink> makeSink() {
        return std::make_unique<UecSink>(nullptr, LINKSPEED, 1.0, Packet::data_packet_size(),
                                         *eventlist_, *nic_, NO_OF_PORTS);
    }
};

// The start time handed to connectPort is an absolute simulation time in
// picoseconds, and must be used as such.
TEST_F(UecSrcTest, ConnectPortStartsFlowAtStartTime) {
    std::unique_ptr<UecSrc>  src1  = makeSrc();
    std::unique_ptr<UecSink> sink1 = makeSink();
    std::unique_ptr<UecSrc>  src2  = makeSrc();
    std::unique_ptr<UecSink> sink2 = makeSink();

    src1->connectPort(0, routeout_, routeback_, *sink1, FIRST_START_PICOSECONDS);
    src2->connectPort(0, routeout_, routeback_, *sink2, SECOND_START_PICOSECONDS);

    EXPECT_TRUE(eventlist_->doNextEvent());
    EXPECT_EQ(eventlist_->now(), FIRST_START_PICOSECONDS);

    EXPECT_TRUE(eventlist_->doNextEvent());
    EXPECT_EQ(eventlist_->now(), SECOND_START_PICOSECONDS);

    EXPECT_FALSE(eventlist_->doNextEvent());
}

// Triggered flows are started by their trigger, not by the event list.
TEST_F(UecSrcTest, ConnectPortDoesNotScheduleTriggeredFlow) {
    std::unique_ptr<UecSrc>  src  = makeSrc();
    std::unique_ptr<UecSink> sink = makeSink();

    src->connectPort(0, routeout_, routeback_, *sink, TRIGGER_START);

    EXPECT_FALSE(eventlist_->doNextEvent());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
