# I love the absurdity of this. Instead of explicitly including our own targets,
# we can only explicitly exclude others. Excluding anything that doesn't begin
# with an 'O' keeps all the 'Ogre...' and 'OpenOBL...' targets but---by
# luck---excludes everything else.
set(GRAPHVIZ_IGNORE_TARGETS "^[^ O].*")
