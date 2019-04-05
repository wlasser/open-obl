# I love the absurdity of this. Instead of explicitly including our own targets,
# we can only explicitly exclude others. Exluding anything that doesn't begin
# with an 'O' keeps all the 'Ogre...' and 'OpenOblivion...' targets but---b
# luck---excludes everything else.
set(GRAPHVIZ_IGNORE_TARGETS "^[^ O].*")
#set(GRAPHVIZ_IGNORE_TARGETS "absl_.*" "pegtl-.*")
