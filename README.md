# AI Toll System

An intelligent toll collection system that leverages computer vision and optical character recognition (OCR) to automate toll processing on highways and urban roadways.

## Features

- **License Plate Recognition**: Automatically detects and reads vehicle license plates
- **Vehicle Classification**: Identifies vehicle types for appropriate toll rates
- **Payment Processing**: Supports multiple payment methods including RFID tags and license plate billing
- **Comprehensive Logging**: Records all transactions and system events
- **Configurable Settings**: Easily adjust toll rates, camera parameters, and system behavior

## System Requirements

- C++17 compatible compiler
- OpenCV 4.x
- Tesseract OCR 4.x
- Leptonica
- Filesystem library support

## Dependencies

- OpenCV: Image processing and computer vision capabilities
- Tesseract: Optical character recognition for license plates
- Leptonica: Image analysis used by Tesseract
- Standard C++ libraries: iostream, filesystem, chrono, etc.

## Installation

1. Install required dependencies:

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install libopencv-dev libtesseract-dev libleptonica-dev

# MacOS
brew install opencv tesseract leptonica
```

2. Clone the repository:

```bash
git clone https://github.com/yourusername/ai-toll-system.git
cd ai-toll-system
```

3. Build the project:

```bash
mkdir build && cd build
cmake ..
make
```

## Directory Structure

```
ai_toll_system/
├── config/                # Configuration files
│   ├── config.txt         # System settings
│   └── registered_vehicles.csv # Vehicle database
├── data/                  # Data files
│   └── tessdata/          # Tesseract language data
├── logs/                  # System logs
│   ├── transaction_log.csv # Record of all transactions
│   └── error_log.txt      # System errors and exceptions
└── output/                # Output files
    ├── captured_plates/   # Cropped license plate images
    ├── processed_images/  # Full vehicle processed images
    └── daily_summaries/   # Daily financial reports
```

## Configuration

The system can be configured by editing the `config.txt` file:

```
# Toll Rates
toll_rate_car=50.0
toll_rate_truck=100.0
toll_rate_bus=75.0

# Camera Settings
camera_resolution_width=1920
camera_resolution_height=1080
camera_fps=30
```

## Usage

1. Run the toll system:

```bash
./ai_toll_system
```

2. For testing with sample images:

```bash
./ai_toll_system --test-image=/path/to/test_vehicle.jpg
```

## Vehicle Registration

Vehicle information is stored in `registered_vehicles.csv` with the following format:

```csv
plate,rfid,balance,type
ABC123,RF1234567,100.0,car
XYZ789,RF7654321,250.0,truck
```

## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- OpenCV community for computer vision tools
- Tesseract OCR project for text recognition
