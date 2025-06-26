
// connections/REST
#include <controllers/LanguagePack.h>
using catena::REST::LanguagePack;

// Initializes the object counter for LanguagePack to 0.
int LanguagePack::objectCounter_ = 0;

LanguagePack::LanguagePack(tcp::socket& socket, ISocketReader& context, IDevice& dm) :
    socket_{socket}, writer_{socket, context.origin()}, context_{context}, dm_{dm} {
    objectId_ = objectCounter_++;
    writeConsole_(CallStatus::kCreate, socket_.is_open());
}

void LanguagePack::proceed() {
    writeConsole_(CallStatus::kProcess, socket_.is_open());

    catena::exception_with_status rc("", catena::StatusCode::OK);
    catena::DeviceComponent_ComponentLanguagePack ans;
    std::shared_ptr<catena::common::Authorizer> sharedAuthz;
    catena::common::Authorizer* authz;

    try {
        // Remove leading "/"
        std::string languageId = context_.fqoid().substr(1);

        // Authz required for all methods except GET.
        if (context_.method() != Method_GET && context_.authorizationEnabled()) {
            // Authorizer throws an error if invalid jws token so no need to handle rc.
            sharedAuthz = std::make_shared<catena::common::Authorizer>(context_.jwsToken());
            authz = sharedAuthz.get();
        } else {
            authz = &catena::common::Authorizer::kAuthzDisabled;
        }

        // GET/language-pack
        if (context_.method() == Method_GET) {
            std::lock_guard lg(dm_.mutex());
            rc = dm_.getLanguagePack(languageId, ans);

        // POST/language-pack and PUT/language-pack
        } else if (context_.method() == Method_POST || context_.method() == Method_PUT) {
            // Constructing the AddLanguagePayload
            catena::AddLanguagePayload payload;
            payload.set_slot(context_.slot());
            payload.set_id(languageId);
            absl::Status status = google::protobuf::util::JsonStringToMessage(absl::string_view(context_.jsonBody()), payload.mutable_language_pack());

            if (status.ok()) {
                std::lock_guard lg(dm_.mutex());
                // Making sure we are only adding with POST and updating with PUT.
                if (context_.method() == Method_POST && dm_.hasLanguage(languageId)
                    || context_.method() == Method_PUT && !dm_.hasLanguage(languageId)) {
                    rc = catena::exception_with_status("", catena::StatusCode::PERMISSION_DENIED);
                } else {
                    // addLanguage ensures shipped languages are not changed.
                    rc = dm_.addLanguage(payload, *authz);
                }
            } else {
                // Failed to parse JSON.
                rc = catena::exception_with_status("", catena::StatusCode::INVALID_ARGUMENT);
            }

        // DELETE/language-pack
        } else if (context_.method() == Method_DELETE) {
            std::lock_guard lg(dm_.mutex());
            rc = dm_.removeLanguage(languageId, *authz);

        // Invalid method.
        } else {
            rc = catena::exception_with_status("", catena::StatusCode::INVALID_ARGUMENT);
        }
    // ERROR
    } catch (catena::exception_with_status& err) {
        rc = catena::exception_with_status(err.what(), err.status);
    } catch (const std::exception& err) {
        rc = catena::exception_with_status(std::string("Error processing request: ") + err.what(), catena::StatusCode::INTERNAL);
    } catch (...) {
        rc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    }

    // Finishing by writing answer to client.
    if (context_.method() == Method_GET) {
        writer_.sendResponse(rc, ans);
    } else {
        // For POST, PUT and DELETE we do not return a message.
        writer_.sendResponse(rc);
    }
}

void LanguagePack::finish() {
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    std::cout << catena::patterns::EnumDecorator<RESTMethod>().getForwardMap().at(context_.method())
              << " LanguagePack[" << objectId_ << "] finished\n";
}
