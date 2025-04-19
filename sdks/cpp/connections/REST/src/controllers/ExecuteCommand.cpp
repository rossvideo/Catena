// connections/REST
#include <controllers/ExecuteCommand.h>
#include <IParam.h>
#include <Device.h>
#include <Status.h>
#include <google/protobuf/util/json_util.h>
#include <iostream>

using catena::REST::ExecuteCommand;
using catena::common::IParam;

// Initializes the object counter for ExecuteCommand to 0.
int ExecuteCommand::objectCounter_ = 0;

ExecuteCommand::ExecuteCommand(tcp::socket& socket, SocketReader& context, Device& dm) :
    socket_{socket}, writer_{socket, context.origin()}, context_{context}, dm_{dm}, ok_{true} {
    objectId_ = objectCounter_++;
    writeConsole(CallStatus::kCreate, socket_.is_open());
    
    try {
        // First try to get the fields
        std::unordered_map<std::string, std::string> fields = {
            {"oid", ""},
            {"slot", ""},
            {"response", ""},
            {"proceed", ""}
        };
        
        context.fields(fields);
        
        // Parse slot number
        try {
            slot_ = fields.at("slot") != "" ? std::stoi(fields.at("slot")) : 0;
        } catch (const std::exception& e) {
            throw catena::exception_with_status("Failed to parse slot number: " + std::string(e.what()), catena::StatusCode::INVALID_ARGUMENT);
        }
        
        oid_ = "/" + fields.at("oid");
        
        // Initialize the request
        req_.set_slot(slot_);
        req_.set_oid(oid_);
        req_.set_respond(fields.at("response") == "true"); 
        req_.set_proceed(fields.at("proceed") == "true"); 

        // Parse JSON body if present
        if (!context.jsonBody().empty()) {
            catena::ExecuteCommandPayload json_payload;
            auto status = google::protobuf::util::JsonStringToMessage(
                absl::string_view(context.jsonBody()), 
                &json_payload
            );
            
            if (status.ok()) {
                if (json_payload.has_value()) {
                    *req_.mutable_value() = json_payload.value();
                }
            } else {
                throw catena::exception_with_status("Failed to parse JSON body: " + std::string(status.message()), catena::StatusCode::INVALID_ARGUMENT);
            }
        }

    } catch (const catena::exception_with_status& e) {
        writer_.write(const_cast<catena::exception_with_status&>(e));
        ok_ = false;
    } catch (const std::exception& e) {
        catena::exception_with_status err("Failed to parse fields: " + std::string(e.what()), catena::StatusCode::INVALID_ARGUMENT);
        writer_.write(err);
        ok_ = false;
    } catch (...) {
        catena::exception_with_status err("Failed to parse fields: unknown error", catena::StatusCode::INVALID_ARGUMENT);
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
            if (req_.respond()) {
                writer_.write(err);
            }
            return;
        }

        // If the command is not found, return an error
        if (command == nullptr) {
            if (req_.respond()) {
                writer_.write(rc);
            }
            return;
        }

        // Execute the command
        catena::CommandResponse res = command->executeCommand(req_.value());

        // Only write response if respond is true
        if (req_.respond()) {
            if (rc.status == catena::StatusCode::OK) {
                catena::Empty ans = catena::Empty();
                writer_.finish(ans);
            } else {
                writer_.write(rc);
            }
        }
    } catch (catena::exception_with_status& err) {
        if (req_.respond()) {
            writer_.write(err);
        }
    } catch (...) {
        if (req_.respond()) {
            catena::exception_with_status err("Unknown error", catena::StatusCode::UNKNOWN);
            writer_.write(err);
        }
    }
}

void ExecuteCommand::finish() {
    writeConsole(CallStatus::kFinish, socket_.is_open());
    std::cout << "ExecuteCommand[" << objectId_ << "] finished\n";
} 