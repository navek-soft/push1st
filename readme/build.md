
### Build from source code

###### Install build environment (Ubuntu)

```bash
	sudo apt install build-essential
```

###### Install dependencies

```bash
	sudo apt install libssl-dev libyaml-cpp-dev liblua5.3-dev
```

###### Make Push1st
```bash
	cd push1st-server
	make build=0 ver=0 bname=src all
```

3th library reference ( json library https://github.com/nlohmann/json )
