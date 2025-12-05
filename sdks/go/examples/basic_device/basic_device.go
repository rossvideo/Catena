package main

import (
	"log"
	"net/http"
	"os"
	"strconv"

	"github.com/rossvideo/catena/sdks/go/pkg/rest"
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
)

func main() {
	assetRoot := envOr("CATENA_ASSET_ROOT", "./assets")
	portStr := envOr("CATENA_PORT", "6254")
	port, err := strconv.Atoi(portStr)
	if err != nil {
		log.Fatalf("invalid CATENA_PORT: %v", err)
	}

	dev := catena.NewSimpleDevice(assetRoot)
	srv := rest.NewServer(dev)

	mux := http.NewServeMux()
	srv.RegisterRoutes(mux)

	addr := ":" + strconv.Itoa(port)
	log.Printf("Catena Go REST listening on %s", addr)
	if err := http.ListenAndServe(addr, mux); err != nil {
		log.Fatal(err)
	}
}

func envOr(key, def string) string {
	if v := os.Getenv(key); v != "" {
		return v
	}
	return def
}
