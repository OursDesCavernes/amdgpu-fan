# amdgpu-fan
Simple daemon to automaticaly manage fans from amdgpu cards

## TODO
* Improve card detection logic
* Add a configuration file
* Add a Unix socket interface for client query and management
* Add a DBus interface for client query and management

## Done
* Basic detection of cards
* Table based regulation
* Cooldown hysteresys function to smooth regulation
* Travis-CI integration
