set -e

USER_NAME=$1
BUILD_TARGET=$2
ROOT_DIR=$3

git submodule update --init --recursive
cd ${ROOT_DIR}/sdks/go
make protos

cd ${ROOT_DIR}
rm -rf ${ROOT_DIR}/go.work ${ROOT_DIR}/go.work.sum
go work init ./sdks/go ./build/go
go work edit -go=1.24
go work edit -toolchain=go1.24.11
go work sync

sudo chown -R ${USER_NAME}:${USER_NAME} ${BUILD_TARGET}
cd ${ROOT_DIR}/sdks/go
go mod tidy
go mod download
make
echo 'Docker setup complete, please restart the dev container!'
  