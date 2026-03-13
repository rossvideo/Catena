module github.com/rossvideo/catena/sdks/go

go 1.24.0

toolchain go1.24.13

require github.com/rossvideo/catena/build/go v0.0.0

require google.golang.org/protobuf v1.36.11

require gopkg.in/yaml.v3 v3.0.1 // indirect

replace github.com/rossvideo/catena/build/go => ../../build/go
