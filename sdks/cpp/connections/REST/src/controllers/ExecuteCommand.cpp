// connections/REST
#include <controllers/ExecuteCommand.h>
#include <IParam.h>
#include <Device.h>
#include <Status.h>

using catena::REST::ExecuteCommand;
using catena::common::IParam;

// Initializes the object counter for ExecuteCommand to 0.
int ExecuteCommand::objectCounter_ = 0;

ExecuteCommand::ExecuteCommand(tcp::socket& socket, SocketReader& context, Device& dm) :
    socket_{socket}, writer_{socket, context.origin()}, context_{context}, dm_{dm}, ok_{true} {
    objectId_ = objectCounter_++;
    writeConsole(CallStatus::kCreate, socket_.is_open());
    // Parsing fields and assigning to respective variables.
    try {
        std::unordered_map<std::string, std::string> fields = {
            {"oid", ""},
            {"slot", ""}
        };
        context.fields(fields);
        slot_ = fields.at("slot") != "" ? std::stoi(fields.at("slot")) : 0;
        oid_ = "/" + fields.at("oid");
        
        // Parse JSON body into ExecuteCommandPayload (?)

    // Parse error
    } catch (...) {
        catena::exception_with_status err("Failed to parse fields", catena::StatusCode::INVALID_ARGUMENT);
        writer_.write(err);
        ok_ = false;
    }
}

void ExecuteCommand::proceed() {
    if (!ok_) { return; }
    writeConsole(CallStatus::kProcess, socket_.is_open());

    catena::exception_with_status rc("", catena::StatusCode::OK);
    try {
        // Get the command and execute it
        std::unique_ptr<IParam> command = nullptr;
        try {
            if (context_.authorizationEnabled()) {
                catena::common::Authorizer authz{context_.jwsToken()};
                command = dm_.getCommand(oid_, rc, authz);
            } else {
                command = dm_.getCommand(oid_, rc, catena::common::Authorizer::kAuthzDisabled);
            }
        } catch (catena::exception_with_status& err) {
            // Likely authentication error, end process
            writer_.write(err);
            return;
        }

        // If the command is not found, return an error
        if (command == nullptr) {
            writer_.write(rc);
            return;
        }

        // Execute the command
        catena::CommandResponse res = command->executeCommand(req_.value());

        // Write the response
        writer_.write(res);
    } catch (catena::exception_with_status& err) {
        writer_.write(err);
    } catch (...) {
        catena::exception_with_status err("Unknown error", catena::StatusCode::UNKNOWN);
        writer_.write(err);
    }
}

void ExecuteCommand::finish() {
    writeConsole(CallStatus::kFinish, socket_.is_open());
    std::cout << "ExecuteCommand[" << objectId_ << "] finished\n";
} 