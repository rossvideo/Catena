module github.com/rossvideo/catena/sdks/go

go 1.23

toolchain go1.24.11

require github.com/rossvideo/catena/build/go v0.0.0

require google.golang.org/protobuf v1.36.11 // indirect

replace github.com/rossvideo/catena/build/go => ../../build/go
