#!/bin/bash

usage() {
  echo "Usage: $0 <alias_name> <command>"
  echo "Arguments:"
  echo "  alias_name: The name of the alias to create"
  echo "  command: The command to associate with the alias"
  echo "Example: $0 myalias 'echo Hello, World!'"
  exit 1
}

make_alias() {
  local alias_name="$1"
  local command="$2"

  if ! grep -Fxq "alias $alias_name=$command" ~/.bashrc; then
    echo "alias $alias_name='$command'" >> ~/.bashrc
    echo "[INFO] Alias '$alias_name' added to ~/.bashrc"
  else
    echo "[INFO] Alias '$alias_name' already exists in ~/.bashrc"
  fi
}


if [[ "$1" == "-h" || "$1" == "--help" ]]; then
  usage
fi

if [[ $# -ne 2 ]]; then
  echo "[ERROR] Invalid number of arguments."
  usage
fi

ALIAS_NAME="$1"
COMMAND="$2"

echo "[INFO] Creating alias '$ALIAS_NAME' for command '$COMMAND'"
make_alias "$ALIAS_NAME" "$COMMAND"
