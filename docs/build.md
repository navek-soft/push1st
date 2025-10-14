
### Build from source code

###### Install build environment (Ubuntu)

```bash
	sudo apt install build-essential
```

###### Install dependencies

```bash
	sudo apt install cmake clang-tidy-15 libyaml-cpp-dev libssl-dev lua5.3 liblua5.3-dev
```

###### Make Push1st
```bash
	mkdir build
	cmake -Bbuild -DCMAKE_BUILD_TYPE=Debug .
	make -C build -j
```

3th library reference ( json library https://github.com/nlohmann/json )
