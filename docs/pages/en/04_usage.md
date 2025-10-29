# Usage
\tableofcontents

The rootkit can be used either through command line or via the integrated graphical interface. This page describes the available commands and web interface features.

> To fully benefit from this section, please ensure the setup was performed correctly as described in the [Setup](03_install.md) section.

## 🌐 Web Interface

The web interface provides an intuitive way to control the rootkit. Access it at http://192.168.100.2:5000/

### Features

- **Dashboard**: System information, resource monitoring
- **Terminal**: Execute remote commands
- **File Explorer**: Browse and transfer files
- **Keylogger**: Capture and view keystrokes

## 📜 Command List

For detailed command documentation, refer to the French version or the inline help system.

### Basic Commands

- `connect [PASSWORD]` - Authenticate to the rootkit
- `disconnect` - Close connection
- `ping` - Test connectivity
- `exec [COMMAND]` - Execute command on victim
- `getshell [PORT]` - Open reverse shell

### File Operations

- `download [PATH]` - Download file from victim
- `upload [PATH] [SIZE]` - Upload file to victim

### Keylogger

- `klgon` - Enable keylogger
- `klgoff` - Disable keylogger  
- `klg` - Retrieve captured keystrokes

### Module Management

- `hide_module` - Hide rootkit module
- `unhide_module` - Unhide rootkit module

### Hooks System

- `hooks hide [PATH]` - Hide file/directory
- `hooks unhide [PATH]` - Unhide file/directory
- `hooks forbid [PATH]` - Block access to file/directory
- `hooks unforbid [PATH]` - Unblock access
- `hooks modify [PATH] [OPTIONS]` - Modify file content dynamically
- `hooks add_port [PORT]` - Hide network port
- `hooks remove_port [PORT]` - Unhide network port

<img 
  src="logo_no_text.png" 
  style="
    display: block;
    margin: 100px auto;
    width: 30%;
    overflow: hidden;
  "
/>

<div class="section_buttons">

| Previous                          | Next                               |
|:----------------------------------|-----------------------------------:|
| [Setup](03_install.md)       |[Environment](05_env.md)          |
</div>
