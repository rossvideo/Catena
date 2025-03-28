
// connections/REST
#include <GetPopulatedSlots.h>

// Initializes the object counter for GetPopulatedSlots to 0.
int API::GetPopulatedSlots::objectCounter_ = 0;

API::GetPopulatedSlots::GetPopulatedSlots(tcp::socket& socket, Device& dm) :
    socket_{socket}, writer_{socket}, dm_{dm} {
    objectId_ = objectCounter_++;
    writeConsole("GetPopulatedSlots", objectId_, CallStatus::kCreate, socket_.is_open());
    // Proceeding with the RPC.
    proceed();
    // Finishing the RPC.
    finish();
}

void API::GetPopulatedSlots::proceed() {
    writeConsole("GetPopulatedSlots", objectId_, CallStatus::kProcess, socket_.is_open());
    try {
        // Getting slot from dm_.
        SlotList slotList;
        slotList.add_slots(dm_.slot());
        // Writing response.
        writer_.write(slotList);
    } catch (...) {
        catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);
        writer_.write(rc);
    }
}

void API::GetPopulatedSlots::finish() {
    writeConsole("GetPopulatedSlots", objectId_, CallStatus::kFinish, socket_.is_open());
    std::cout << "GetPopulatedSlots[" << objectId_ << "] finished\n";
}
