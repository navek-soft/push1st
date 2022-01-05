## Installation guide

###### Import repository key

```bash
	wget https://nexus.naveksoft.com/repository/gpg/naveksoft.gpg.key -O naveksoft.gpg.key
	sudo apt-key add naveksoft.gpg.key
```

###### Add repository to source list
```bash
	echo "deb [arch=amd64] https://nexus.naveksoft.com/repository/ubuntu-universe/ universe main" | sudo tee /etc/apt/sources.list.d/naveksoft-universe.list
```


###### Check available versions
```bash
	sudo apt update
	apt list -a push1st
```

###### Install from repository

```bash
	sudo apt update
	sudo apt install push1st
```

## Install additional dependencies

###### Lua 5.3 dependencies (optional)

```bash
	sudo luarocks install lua-cjson2
```
