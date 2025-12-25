This is a C++-based repository for a cycle computer. It is primarily responsible for ingesting sensor data, displaying it on a monitor, and logging the data. Please follow these guidelines when contributing:

## Code Standards

### Repository Structure
- `CMakeLists.txt`: Top-level CMake configuration for building the project
- `main.cc`: Application entry point
- `src/`: C++ source files
	- `core/`: Core application logic
	- `display/`: Display drivers, rendering and touch controller integration
	- `hal/`: Hardware abstraction layer (GPIO, I2C, SPI, UART)
	- `sensor/`: Sensor integrations
	- `third_party/`: Third-party source integrations
	- `util/`: Utilities
- `include/`: Public headers matching `src/` layout
- `config/`: Configuration files
- `resource/`: Static assets (background images used by the UI)
- `scripts/`: Build and helper scripts
- `tests/`: Test scaffolding and suites organized by subsystem
- `build/`: Generated build artifacts (CMake cache, Ninja files, object dirs)
- `doc/`: Project documentation
- `try/`: Experimental prototypes and playground (This dir is independent of the project)
- `log/`: Runtime logs
- `README.md`: Project overview and quickstart

## Key Guidelines
1. Follow C++ best practices and idiomatic patterns
2. Basically, follow Google C++ style Guide <https://google.github.io/styleguide/cppguide.html>
3. Maintain existing code structure and organization
4. Use dependency injection patterns where appropriate
<!-- 5. Follow doc.instruction.md when writing class design documents in Markdown -->
<!-- 5. Write unit tests for new functionality. For writing tests, follow the instructions in test.instruction.md -->