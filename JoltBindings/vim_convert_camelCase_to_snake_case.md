# Full match
```
%s/\l\u/\= join(split(tolower(submatch(0)), '\zs'), '_')/gc
```
