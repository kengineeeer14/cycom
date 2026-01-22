#!/bin/bash

# Git prompt support
if [ -f /etc/bash_completion.d/git-prompt.sh ]; then
    source /etc/bash_completion.d/git-prompt.sh
fi

# Custom PS1 with git branch display
PS1='🚴 \[\e[1;33m\]\u\[\e[m\] \[\e[1;33m\]\t:\[\e[m\] \[\e[1;34m\]\w\[\e[m\]$(__git_ps1 " (%s)")\[\e[1;33m\]\$ \[\e[m\]'
