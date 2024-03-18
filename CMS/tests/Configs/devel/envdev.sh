export LC_CTYPE=en_US.UTF-8
export LC_ALL=en_US.UTF-8

export user=$(id -u -n)
export usergroup=$(id -g -n)

# source root's
export unixcommons_root=$HOME/projects/unixcommons_cobrazz
export server_root=$HOME/projects/rtbserver_cobrazz

# cluster directories
export config_root=$HOME/run/etc/
export workspace_root=$HOME/run/var/

export PATH=$PATH:$server_root/bin
export PATH=$PATH:$server_root/ConfigSys
export PATH=$PATH:$server_root/build/bin
export VERSION='4.0.0.19'
export LOG_LEVEL=7
