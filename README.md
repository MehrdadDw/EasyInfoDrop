# EasyInfoDrop

EasyInfoDrop is a lightweight, Qt-based application designed to streamline your workflow when dealing with repetitive text, such as names, emails, or cover letter snippets. Whether you're filling out web forms, drafting emails, or preparing documents in tools like Microsoft Word, EasyInfoDrop lets you drag and drop pre-configured text directly into your target application, saving time and reducing errors. With a customizable configuration file, you can store and manage frequently used text snippets effortlessly.

## Features

- **Drag-and-Drop Simplicity**: Select text from a list and drag it into web forms, text editors, or any application that accepts text input.
- **Configurable Text List**: Define your own text snippets (e.g., name, email, cover letter text) in a simple JSON configuration file.
- **Clipboard Integration**: Click or drag items to copy them to your clipboard, with automatic paste simulation on Linux (X11) for seamless integration.
- **Pin Window Option**: Keep the application on top of other windows for quick access during repetitive tasks.
- **Cross-Platform Potential**: Built with Qt for easy adaptation to Linux, Windows, and macOS (currently optimized for Linux with X11).

## Installation

Download the latest release from the [Releases page](https://github.com/MehrdadDw/EasyInfoDrop/releases).

## Usage

To run EasyInfoDrop from a release tarball:

```bash
tar -xzvf release-easy-info-drop-<version>.tar.gz
chmod +x EasyInfoDrop
./EasyInfoDrop
```

Replace `<version>` with the specific release version (e.g., `v1.0.5`).

## Configuration

Customize your text snippets by editing `config/config.json`. The default configuration includes common fields like name and email:

```json
{
  "items": [
    {
      "name": "Full Name",
      "value": "John Doe"
    },
    {
      "name": "Email",
      "value": "john@example.com"
    },
    {
      "name": "Name",
      "value": "John"
    },
    {
      "name": "Last Name",
      "value": "Doe"
    }
  ]
}
```

- **Fields**:
  - `name`: The label shown in the list (e.g., `Full Name`).
  - `value`: The text copied or dragged (e.g., `John Doe`).
- **Editing**: Click the "Edit" button in the app to open `config.json` in a text editor, or modify it manually.
- **Refreshing**: Use the "Refresh" button to reload changes without restarting the app.

## Build and Develop

To build EasyInfoDrop from source:

```bash
sudo apt install qtbase5-dev qt5-qmake libqt5widgets5 libx11-dev cmake build-essential
mkdir build
cd build
cmake ..
make
./EasyInfoDrop
```

### Dependencies
- Qt 5
- X11 (for clipboard paste simulation on Linux)
- nlohmann/json (included in source)
- CMake (for building)

## Why Use EasyInfoDrop?

EasyInfoDrop is perfect for professionals, students, or anyone who frequently reuses text snippets. For example:
- **Job Applications**: Drag your name, email, or cover letter text into online forms or Word documents.
- **Customer Support**: Quickly insert standard responses or contact details.
- **Data Entry**: Avoid retyping repetitive information like addresses or IDs.

Say goodbye to manual copying and pasting—EasyInfoDrop makes repetitive text tasks fast, accurate, and hassle-free!

## Contributing

Contributions are welcome! Fork the repository, make your changes, and submit a pull request. For issues or feature requests, please use the [Issues page](https://github.com/MehrdadDw/EasyInfoDrop/issues).

## License

This project is licensed under the MIT License—see the [LICENSE](LICENSE) file for details.
