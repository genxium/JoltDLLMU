As I live in China and use Docker Desktop on Windows 11, so I edited `$env:USERPROFILE/.docker/daemon.js` and `$env:USERPROFILE/.docker/windows-daemon.js` to add some registries.
```
{
    ...,
    "registry-mirrors": [
    	"https://docker-0.unsee.tech",
        "https://docker-cf.registry.cyou",
        "https://docker.1panel.live"
    ],
    ...
}
```

Then restart dockerd to ensure that the new configs are effective. 
