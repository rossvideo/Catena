// connections/REST
#include <controllers/ExecuteCommand.h>
#include <Status.h>
#include <google/protobuf/util/json_util.h>
#include <iostream>

using catena::REST::ExecuteCommand;
using catena::common::IParam;
using catena::common::IParamDescriptor;

// Initializes the object counter for ExecuteCommand to 0.
int ExecuteCommand::objectCounter_ = 0;

ExecuteCommand::ExecuteCommand(tcp::socket& socket, ISocketReader& context, IDevice& dm) :
    socket_{socket}, writer_{socket, context.origin()}, context_{context}, dm_{dm} {
    objectId_ = objectCounter_++;
    writeConsole_(CallStatus::kCreate, socket_.is_open());
}

void ExecuteCommand::proceed() {
    writeConsole_(CallStatus::kProcess, socket_.is_open());

    catena::exception_with_status rc("", catena::StatusCode::OK);
    bool respond = context_.hasField("respond");
    bool proceed = context_.hasField("proceed");

    try {
        // Parse JSON body if not empty.
        catena::Value val;
        if (!context_.jsonBody().empty()) {
            auto status = google::protobuf::util::JsonStringToMessage(absl::string_view(context_.jsonBody()), &val);
            if (!status.ok()) {
                rc = catena::exception_with_status("Failed to parse JSON body", catena::StatusCode::INVALID_ARGUMENT);
            }
        }
        
        // Continue if the jsonBody was successfully parsed.
        if (rc.status == catena::StatusCode::OK) {
            // Get the command.
            std::unique_ptr<IParam> command = nullptr;
            if (context_.authorizationEnabled()) {
                catena::common::Authorizer authz{context_.jwsToken()};
                command = dm_.getCommand(context_.fqoid(), rc, authz);
            } else {
                command = dm_.getCommand(context_.fqoid(), rc, catena::common::Authorizer::kAuthzDisabled);
            }

            // If the command is not found, return an error
            if (command != nullptr) {
                // Execute the command and write response if respond = true.
                std::unique_ptr<IParamDescriptor::ICommandResponder> responder = command->executeCommand(val);
                if (!responder) {
                    rc = catena::exception_with_status("Illegal state", catena::StatusCode::INTERNAL);
                } else {
                    while (responder->hasMore()) {
                        writeConsole_(CallStatus::kWrite, socket_.is_open());
                        try {
                            catena::CommandResponse res = responder->getNext();
                            if (respond) {
                                writer_.sendResponse(rc, res);
                            }
                        } catch (const catena::exception_with_status& err) {
                            rc = catena::exception_with_status(err.what(), err.status);
                            break;
                        } catch (...) {
                            rc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
                            break;
                        }
                    }
                }
            }
        }
    } catch (catena::exception_with_status& err) {
        rc = catena::exception_with_status(err.what(), err.status);
    } catch (...) {
        rc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    }
    // Writing final code if respond = false or an error occurred.
    if (rc.status != catena::StatusCode::OK || !respond) {
        writer_.sendResponse(rc);
    }
}

void ExecuteCommand::finish() {
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    std::cout << "ExecuteCommand[" << objectId_ << "] finished\n";
} 
