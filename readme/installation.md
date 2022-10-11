## Installation guide

###### Import repository key

```bash
	apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 9A4D3B9B041D12EF0D23694D8222A313EDE992FD
```

###### Add repository to source list
```bash
	echo "deb [arch=amd64] https://nexus.naveksoft.com/repository/ubuntu-universe/ universe main" | sudo tee /etc/apt/sources.list.d/naveksoft-universe.list
	echo "machine nexus.naveksoft.com/repository login public password public" | sudo tee /etc/apt/auth.conf.d/nexus.naveksoft.com.conf
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
