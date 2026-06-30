As I live in China and use `Docker Desktop on Windows 11`, `$env:USERPROFILE/.docker/daemon.json` and `$env:USERPROFILE/.docker/windows-daemon.json` are edited locally to add some registries (or if you're using `Docker Desktop v4.77.0+ on Windows` follow its corresponding UI setup guidance for "registry-mirrors").

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
