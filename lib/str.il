Compiled Ruleset @602 
Compiled Rule 
	--atommatch:
		spec           [2, 2]
	--memmatch:
		spec           [1, 1]
		jump           [L260, [0], [], []]
	--guard:L260:
		spec           [1, 1]
		jump           [L253, [0], [], []]
	--body:L253:
		spec           [1, 6]
		commit         ["_init", 0]
		newmem      [1, 0, 0]
		loadruleset    [1, @601]
		newatom     [2, 1, 'str'_1]
		newatom     [3, 1, 'module'_1]
		alloclink   [4, 2, 0]
		alloclink   [5, 3, 0]
		unifylinks     [4, 5, 1]
		enqueueatom    [3]
		enqueueatom    [2]
		proceed        []


Compiled Ruleset @601      
Compiled Rule 
	--atommatch:
		spec           [2, 4]
	--memmatch:
		spec           [1, 6]
		findatom    [1, 0, 'str'.'from'_2]
		derefatom   [2, 1, 0]
		isunary        [2]
		commit         ["_Hstr", 0]
		dequeueatom    [1]
		removeatom     [1, 0, 'str'.'from'_2]
		dequeueatom    [2]
		removeatom     [2, 0]
		copyatom    [3, 0, 2]
		newatom     [4, 0, 'string_make'_1]
		newatom     [5, 0, '$callback'_3]
		newlink        [4, 0, 5, 0, 0]
		newlink        [5, 1, 3, 0, 0]
		relink         [5, 2, 1, 1, 0]
		enqueueatom    [5]
		enqueueatom    [4]
		callback       [0, 5]
		freeatom       [1]
		freeatom       [2]
		proceed        []

Compiled Rule 
	--atommatch:
		spec           [2, 5]
	--memmatch:
		spec           [1, 8]
		findatom    [1, 0, 'str'.'concat'_3]
		derefatom   [3, 1, 1]
		isstring       [3]
		derefatom   [2, 1, 0]
		isstring       [2]
		commit         ["_Hstr", 0]
		dequeueatom    [1]
		removeatom     [1, 0, 'str'.'concat'_3]
		dequeueatom    [3]
		removeatom     [3, 0]
		dequeueatom    [2]
		removeatom     [2, 0]
		copyatom    [4, 0, 3]
		copyatom    [5, 0, 2]
		newatom     [6, 0, 'string_concat'_1]
		newatom     [7, 0, '$callback'_4]
		newlink        [6, 0, 7, 0, 0]
		newlink        [7, 1, 5, 0, 0]
		newlink        [7, 2, 4, 0, 0]
		relink         [7, 3, 1, 2, 0]
		enqueueatom    [7]
		enqueueatom    [6]
		callback       [0, 7]
		freeatom       [1]
		freeatom       [3]
		freeatom       [2]
		proceed        []

Compiled Rule 
	--atommatch:
		spec           [2, 5]
	--memmatch:
		spec           [1, 8]
		findatom    [1, 0, 'str'.'substr'_3]
		derefatom   [3, 1, 1]
		isint          [3]
		derefatom   [2, 1, 0]
		isstring       [2]
		commit         ["_Hstr", 0]
		dequeueatom    [1]
		removeatom     [1, 0, 'str'.'substr'_3]
		dequeueatom    [3]
		removeatom     [3, 0]
		dequeueatom    [2]
		removeatom     [2, 0]
		copyatom    [4, 0, 3]
		copyatom    [5, 0, 2]
		newatom     [6, 0, 'string_substr_right'_1]
		newatom     [7, 0, '$callback'_4]
		newlink        [6, 0, 7, 0, 0]
		newlink        [7, 1, 5, 0, 0]
		newlink        [7, 2, 4, 0, 0]
		relink         [7, 3, 1, 2, 0]
		enqueueatom    [7]
		enqueueatom    [6]
		callback       [0, 7]
		freeatom       [1]
		freeatom       [3]
		freeatom       [2]
		proceed        []

Compiled Rule 
	--atommatch:
		spec           [2, 6]
	--memmatch:
		spec           [1, 10]
		findatom    [1, 0, 'str'.'substr'_4]
		derefatom   [4, 1, 2]
		isint          [4]
		derefatom   [3, 1, 1]
		isint          [3]
		derefatom   [2, 1, 0]
		isstring       [2]
		commit         ["_Hstr", 0]
		dequeueatom    [1]
		removeatom     [1, 0, 'str'.'substr'_4]
		dequeueatom    [3]
		removeatom     [3, 0]
		dequeueatom    [2]
		removeatom     [2, 0]
		dequeueatom    [4]
		removeatom     [4, 0]
		copyatom    [5, 0, 3]
		copyatom    [6, 0, 2]
		copyatom    [7, 0, 4]
		newatom     [8, 0, 'string_substr'_1]
		newatom     [9, 0, '$callback'_5]
		newlink        [8, 0, 9, 0, 0]
		newlink        [9, 1, 6, 0, 0]
		newlink        [9, 2, 5, 0, 0]
		newlink        [9, 3, 7, 0, 0]
		relink         [9, 4, 1, 3, 0]
		enqueueatom    [9]
		enqueueatom    [8]
		callback       [0, 9]
		freeatom       [1]
		freeatom       [3]
		freeatom       [2]
		freeatom       [4]
		proceed        []

Compiled Rule 
	--atommatch:
		spec           [2, 4]
	--memmatch:
		spec           [1, 6]
		findatom    [1, 0, 'str'.'length'_2]
		derefatom   [2, 1, 0]
		isstring       [2]
		commit         ["_Hstr", 0]
		dequeueatom    [1]
		removeatom     [1, 0, 'str'.'length'_2]
		dequeueatom    [2]
		removeatom     [2, 0]
		copyatom    [3, 0, 2]
		newatom     [4, 0, 'string_length'_1]
		newatom     [5, 0, '$callback'_3]
		newlink        [4, 0, 5, 0, 0]
		newlink        [5, 1, 3, 0, 0]
		relink         [5, 2, 1, 1, 0]
		enqueueatom    [5]
		enqueueatom    [4]
		callback       [0, 5]
		freeatom       [1]
		freeatom       [2]
		proceed        []

Compiled Rule 
	--atommatch:
		spec           [2, 4]
	--memmatch:
		spec           [1, 6]
		findatom    [1, 0, 'str'.'reverse'_2]
		derefatom   [2, 1, 0]
		isstring       [2]
		commit         ["_Hstr", 0]
		dequeueatom    [1]
		removeatom     [1, 0, 'str'.'reverse'_2]
		dequeueatom    [2]
		removeatom     [2, 0]
		copyatom    [3, 0, 2]
		newatom     [4, 0, 'string_reverse'_1]
		newatom     [5, 0, '$callback'_3]
		newlink        [4, 0, 5, 0, 0]
		newlink        [5, 1, 3, 0, 0]
		relink         [5, 2, 1, 1, 0]
		enqueueatom    [5]
		enqueueatom    [4]
		callback       [0, 5]
		freeatom       [1]
		freeatom       [2]
		proceed        []


Module
'str' {@601}
Inline
