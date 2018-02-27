#! /bin/bash
#
# Install all the git repositories inclunding RIOT master branch

# Default directory for all forks of RIOT other than (RIOT-OS)/master
RIOT_BETA="betas"


USR1_RIOT="https://github.com/rfuentess/riot.git"
USR1_VER1="alpha/sock/secure"

USR2_RIOT="https://github.com/miri64/riot.git"
USR2_VER1="gnrc_sock/feat/async"

#######################################
# Remove all the alternatiove git repositories.
# Globals:
#   None
# Arguments:
#   None
# Returns:
#  None
#######################################
cleanup(){
    rm -rf betas/
}

#######################################
# Clone the repositories
# Globals:
#   USR*
#   RIOT_*
# Arguments:
#   None
# Returns:
#  None
#######################################
generate(){

    echo "Loading offical RIOT master branch..."
    # The official RIOT repository
    git submodule init
    git submodule update

    echo "Loading beta RIOT branches..."
    # Betas repositories
    mkdir -p betas
    git clone "${USR1_RIOT}" -b "${USR1_VER1}" "${RIOT_BETA}"/async
}

    cleanup ""
    generate ""
