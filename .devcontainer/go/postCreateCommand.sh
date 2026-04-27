set -e

USER_NAME=$1
BUILD_TARGET=$2
ROOT_DIR=$3

cd ${ROOT_DIR}/sdks/go
make protos-all

mkdir -p ${BUILD_TARGET}
sudo chown -R ${USER_NAME}:${USER_NAME} ${BUILD_TARGET}
cd ${ROOT_DIR}/sdks/go
go mod tidy
go mod download
make
echo 'Docker setup complete, please restart the dev container!'
