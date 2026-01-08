package internal

import (
	"encoding/json"
	"io"
	"net/http"

	"google.golang.org/protobuf/encoding/protojson"

	"github.com/rossvideo/catena/build/go/protos"
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
)

func ReadRequestJSON(r *http.Request) (*protos.Value, error) {
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
	if res.Payload == nil {
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(res.Status)
		return
	}

	b, err := (protojson.MarshalOptions{
		UseProtoNames:   true,
		EmitUnpopulated: false,
	}).Marshal(res.Payload)
	if err != nil {
		// Marshaling failed (e.g., invalid UTF-8). Don't return a 2xx with an empty body.
		// Prefer a JSON error body so clients that always parse JSON don't explode.
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusInternalServerError)
		_ = json.NewEncoder(w).Encode(map[string]string{
			"error": "failed to marshal response payload",
		})
		return
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(res.Status)
	_, _ = w.Write(b)
}
