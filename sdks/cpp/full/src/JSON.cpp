#include <JSON.h>

#include <google/protobuf/util/json_util.h>


std::string catena::full::printJSON(const google::protobuf::Message& m) {
    std::string str;
    google::protobuf::util::JsonPrintOptions jopts{};
    jopts.add_whitespace = true;
    jopts.preserve_proto_field_names = true;
    auto status = google::protobuf::util::MessageToJsonString(m, &str, jopts);
    assert(status.ok());
    return str;
}

void catena::full::parseJSON(const std::string& msg, google::protobuf::Message& m) {
    google::protobuf::util::JsonParseOptions jopts{};
    auto status = google::protobuf::util::JsonStringToMessage(msg, &m, jopts);
    assert(status.ok());
}
