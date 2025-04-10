
// connections/REST
#include <controllers/GetPopulatedSlots.h>
using catena::REST::GetPopulatedSlots;

// Initializes the object counter for GetPopulatedSlots to 0.
int GetPopulatedSlots::objectCounter_ = 0;

GetPopulatedSlots::GetPopulatedSlots(tcp::socket& socket, SocketReader& context, Device& dm) :
    socket_{socket}, writer_{socket, context.origin()}, dm_{dm} {
    objectId_ = objectCounter_++;
    writeConsole(CallStatus::kCreate, socket_.is_open());
}

void GetPopulatedSlots::proceed() {
    writeConsole(CallStatus::kProcess, socket_.is_open());
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

void GetPopulatedSlots::finish() {
    writeConsole(CallStatus::kFinish, socket_.is_open());
    std::cout << "GetPopulatedSlots[" << objectId_ << "] finished\n";
}
