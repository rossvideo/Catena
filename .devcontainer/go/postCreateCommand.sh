set -e

USER_NAME=$1
BUILD_TARGET=$2
ROOT_DIR=$3

cd ${ROOT_DIR}/sdks/go
make protos-all

cd ${ROOT_DIR}
# should exist cause of volume mount, but just in case
mkdir -p ${BUILD_TARGET}
# volume mount might belong to root
sudo chown -R ${USER_NAME}:${USER_NAME} ${BUILD_TARGET}

cd ${ROOT_DIR}/sdks/go
go mod tidy
go mod download
make
echo 'Docker setup complete, please restart the dev container!'
