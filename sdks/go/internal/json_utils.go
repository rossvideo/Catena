package internal

import (
	"io"
	"net/http"

	"google.golang.org/protobuf/encoding/protojson"

	"github.com/rossvideo/catena/build/go/protos"
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
)

func ReadValueFromRequest(r *http.Request) (*protos.Value, error) {
	defer r.Body.Close()

	data, err := io.ReadAll(r.Body)
	if err != nil || err == io.EOF {
		return nil, err
	}

	v := &protos.Value{}
	if err := (protojson.UnmarshalOptions{
		DiscardUnknown: true,
	}).Unmarshal(data, v); err != nil {
		return nil, err
	}
	return v, nil
}

func WriteResponseJSON(w http.ResponseWriter, res catena.StatusResult) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(res.Status)

	if res.Payload == nil {
		return
	}

	b, err := (protojson.MarshalOptions{
		UseProtoNames:   true,
		EmitUnpopulated: false,
	}).Marshal(res.Payload)
	if err != nil {
		return
	}
	_, _ = w.Write(b)
}
