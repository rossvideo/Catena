
// connections/REST
#include <controllers/GetPopulatedSlots.h>
using catena::REST::GetPopulatedSlots;

// Initializes the object counter for GetPopulatedSlots to 0.
int GetPopulatedSlots::objectCounter_ = 0;

GetPopulatedSlots::GetPopulatedSlots(tcp::socket& socket, ISocketReader& context, SlotMap& dms) :
    socket_{socket}, writer_{socket, context.origin()}, dms_{dms} {
    objectId_ = objectCounter_++;
    writeConsole_(CallStatus::kCreate, socket_.is_open());
}

void GetPopulatedSlots::proceed() {
    writeConsole_(CallStatus::kProcess, socket_.is_open());
    // Getting slot from dm.
    SlotList slotList;
    for (auto [slot, dm] : dms_) {
        // If a devices exists at the slot, add it to the response.
        if (dm) {
            slotList.add_slots(slot);
        }
    }
    // Writing response.
    writer_.sendResponse(catena::exception_with_status("", catena::StatusCode::OK), slotList);

    // Writing the final status to the console.
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    DEBUG_LOG << "GetPopulatedSlots[" << objectId_ << "] finished\n";
}
