===building===

1) prepare skia
see https://skia.org/docs/user/download/
    * clone skia into {projDir}/skia folder

    * build skia:
```
gn gen out/Shared --args='is_official_build=false is_component_build=true'
ninja -C out/Shared
```


2) build the test example \
```
mkdir build
cd build
cmake ..
make
```

3) run \
perhaps you need set add "export DISPLAY=$(hostname).local:0" into your .bashrc
also you can test UI by xeyes utily (sudo apt install xeyes)
