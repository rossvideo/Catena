Write-Host "`n`n`n`n`n`n"

# Define variables
$defaultUser =  $($args[0])
$gitName = $($args[1])
$gitEmail = $($args[2])

$basewslDistro = "Ubuntu-24.04"
$wslDistro =  "CatenaUbuntu"
$installPath = "C:\WSL\$wslDistro"
$exportPath = "C:\WSL\Ubuntu24.tar"
$logFile = "C:\WSL\wsl_log.txt"


# create WSL folder if doesn't exists
if (-not (Test-Path "C:\WSL")) {
	mkdir "C:\WSL"
}

"--- WSL Log Started: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss') ---`n" | Out-File -FilePath $logFile -Force

function stage { # Function to handle the progress bar
    param (
        [int]$PerStage,
        [string]$message
    )
    Write-Progress -Activity "Progress: (Get-Content -Path '$logFile' -Wait )" -Status $message -PercentComplete $PerStage
}

function Run-WSLCommand { # Function to run commands in WSL
    param (
        [int]$PerStage,
        [string]$message,
        [string]$command
    )
    Write-Host "++++++++++++ [$PerStage] $message ++++++++++++" *>> C:\WSL\wsl_log.txt
    Write-Host "$command"
    stage $PerStage $message
    wsl -d $wslDistro --user $defaultUser -- bash -c "$command" *>> C:\WSL\wsl_log.txt
    Write-Host "----------- [$PerStage] $message -----------" *>> C:\WSL\wsl_log.txt
}

stage 1 "Setting up WSL"

# Check if WSL is installed
if (-not (Get-Command "wsl" -ErrorAction SilentlyContinue)) {
    Write-Output "Installing WSL..."
    wsl --install
    Write-Output "WSL installed. Please restart your PC and rerun this script."
    Pause
    exit
}
stage 2 "Setting up WSL"
# Check if $basewslDistro is installed
$wslList = wsl --list --quiet
if ($wslList -notcontains "$basewslDistro") {
    Write-Output "Installing $basewslDistro..."
    wsl --install -d $basewslDistro
}else{
    Write-Output "this will kill the $basewslDistro wsl so you can do that if you want to use this script"
    Write-Output "wsl --unregister $basewslDistro"
    Pause
    exit
}
stage 3 "Setting up WSL"
if ($wslList -contains $wslDistro) {
    Write-Output "WSL instance '$wslDistro' already exists.  Removing it"
    wsl --unregister $wslDistro
}
stage 4 "Setting up WSL"
$wslList = wsl --list --quiet
# Check if the custom distro exists
if ($wslList -notcontains $wslDistro) {
    Write-Output "Creating a labeled WSL instance '$wslDistro'..."
	
    # Export and import $basewslDistro with a new name
    wsl --export $basewslDistro $exportPath
    wsl --unregister $basewslDistro
    stage 6 "Setting up WSL"
    wsl --import $wslDistro $installPath $exportPath

    # Start WSL and create a default user
    wsl -d $wslDistro -- bash -c "adduser --disabled-password --gecos '' $defaultUser && echo '$defaultUser ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers"
    wsl -d $wslDistro -- bash -c "echo -e '[user]\ndefault=$defaultUser' | sudo tee -a /etc/wsl.conf"
    stage 9 "Setting up WSL"
    Write-Output "WSL setup complete. Restarting WSL..."
    wsl --shutdown
} else {
    Write-Output "Somthing has gone wrong, please remove the old wsl your self"
    Write-Output "wsl --unregister $wslDistro"

    Pause
    exit
}

clear
Write-Host "`n`n`n`n`n`n"

## see readme.md

# for ubuntu 24.04
## setup the certs for docker
Run-WSLCommand 10 "Dependencies cmake and npm" "sudo apt-get update && sudo apt-get -y install build-essential cmake nodejs npm"

Run-WSLCommand 11 "Install ca-certificates curl" "sudo apt-get update && sudo apt-get install ca-certificates curl"
Run-WSLCommand 12 "Install keyrings" "sudo install -m 0755 -d /etc/apt/keyrings"
Run-WSLCommand 13 "Download Docker's GPG key" "sudo curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc"
Run-WSLCommand 14 "Add read permissions to GPG key" "sudo chmod a+r /etc/apt/keyrings/docker.asc"

# add docker repo
$dockerCommand = 'echo \deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu $(. /etc/os-release && echo ${UBUNTU_CODENAME:-$VERSION_CODENAME}) stable\ | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null'
Run-WSLCommand 15 "Add Docker Repo" $dockerCommand
Run-WSLCommand 16 "Install Docker" "sudo apt-get update && sudo apt-get install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin"

# docker settings
Run-WSLCommand 17 "usermod docker" "sudo usermod -aG docker $defaultUser"
Run-WSLCommand 18 "enable docker.service" "sudo systemctl enable docker.service"
Run-WSLCommand 19 "enable containerd.service" "sudo systemctl enable containerd.service"
Run-WSLCommand 20 "Start Docker" "sudo service docker start"

Run-WSLCommand 21 "Build Catena build dir" "mkdir -p ~/Catena/${BUILD_TARGET}"


Run-WSLCommand 22 "Pull Catena repo" "mkdir -p ~/Catena && cd ~/Catena && git init && git remote add origin https://github.com/rossvideo/Catena.git && git pull origin develop --recurse-submodules && git checkout develop"

# set git to use my username and email
Run-WSLCommand 23 "Git config user info" "git config --global user.email '$gitEmail' && git config --global user.name '$gitName'"

# start cursor
Run-WSLCommand 25 "Start Cursor" "cd ~/Catena && code ."

