# Full match
```
%s/\l\u/\= join(split(tolower(submatch(0)), '\zs'), '_')/gc

# Convert each name_like_this to NameLikeThis in current line.
```
:%s#\(\%(\<\l\+\)\%(_\)\@=\)\|_\(\l\)#\u\1\2#gc
```

# Convert each name_like_this to nameLikeThis in current line.
```
:%s#_\(\l\)#\u\1#gc
```
