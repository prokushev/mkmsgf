; Message data area                     
TXT_MSG_FILE_NOT_FOUND	LABEL	WORD
		DW	END_MSG_FILE_NOT_FOUND - TXT_MSG_FILE_NOT_FOUND - 2
		DB	'MAB0100: '
		DB	'File not found',0DH,0AH
END_MSG_FILE_NOT_FOUND		LABEL	WORD
		DB		0
TXT_MSG_BLANK_NOT_USED1	LABEL	WORD
		DW	END_MSG_BLANK_NOT_USED1 - TXT_MSG_BLANK_NOT_USED1 - 2
		DB	'MAB0101:
		DB	'
'
END_MSG_BLANK_NOT_USED1		LABEL	WORD
		DB		0
TXT_MSG_USAGE_MESSAGE	LABEL	WORD
		DW	END_MSG_USAGE_MESSAGE - TXT_MSG_USAGE_MESSAGE - 2
		DB	'MAB0102: '
		DB	'Usage: del [driv'
		DB	'e:][path] filena'
		DB	'me',0DH,0AH
END_MSG_USAGE_MESSAGE		LABEL	WORD
		DB		0
TXT_MSG_BLANK_NOT_USED2	LABEL	WORD
		DW	END_MSG_BLANK_NOT_USED2 - TXT_MSG_BLANK_NOT_USED2 - 2
		DB	'MAB0103:
		DB	'
'
END_MSG_BLANK_NOT_USED2		LABEL	WORD
		DB		0
TXT_MSG_FILE_COPIED	LABEL	WORD
		DW	END_MSG_FILE_COPIED - TXT_MSG_FILE_COPIED - 2
		DB	'%1 files copied',0DH,0AH
END_MSG_FILE_COPIED		LABEL	WORD
		DB		0
TXT_MSG_DATA_WARNING	LABEL	WORD
		DW	END_MSG_DATA_WARNING - TXT_MSG_DATA_WARNING - 2
		DB	'MAB0105: '
		DB	'Warning! All dat'
		DB	'a will be destro'
		DB	'yed!',0DH,0AH
END_MSG_DATA_WARNING		LABEL	WORD
		DB		0
TXT_MSG_BLANK_NOT_USED3	LABEL	WORD
		DW	END_MSG_BLANK_NOT_USED3 - TXT_MSG_BLANK_NOT_USED3 - 2
		DB	'MAB0106:
		DB	'
'
END_MSG_BLANK_NOT_USED3		LABEL	WORD
		DB		0
TXT_MSG_BLANK_NOT_USED4	LABEL	WORD
		DW	END_MSG_BLANK_NOT_USED4 - TXT_MSG_BLANK_NOT_USED4 - 2
		DB	'MAB0107:
		DB	'
'
END_MSG_BLANK_NOT_USED4		LABEL	WORD
		DB		0
TXT_MSG_APPLY_PATCHES	LABEL	WORD
		DW	END_MSG_APPLY_PATCHES - TXT_MSG_APPLY_PATCHES - 2
		DB	'Do you wish to a'
		DB	'pply these patch'
		DB	'es (Y or N)? '
END_MSG_APPLY_PATCHES		LABEL	WORD
		DB		0
TXT_MSG_DIVIDE_OVERFLOW	LABEL	WORD
		DW	END_MSG_DIVIDE_OVERFLOW - TXT_MSG_DIVIDE_OVERFLOW - 2
		DB	'MAB0109: '
		DB	'Divide overflow',0DH,0AH
END_MSG_DIVIDE_OVERFLOW		LABEL	WORD
		DB		0
TXT_MSG_PROMPT_TEST	LABEL	WORD
		DW	END_MSG_PROMPT_TEST - TXT_MSG_PROMPT_TEST - 2
		DB	'Prompt with no 0'
		DB	' at end',0DH,0AH
END_MSG_PROMPT_TEST		LABEL	WORD
		DB		0
TXT_MSG_END_TEST_1	LABEL	WORD
		DW	END_MSG_END_TEST_1 - TXT_MSG_END_TEST_1 - 2
		DB	'Info with 0 at e'
		DB	'nd'
END_MSG_END_TEST_1		LABEL	WORD
		DB		0
TXT_MSG_END_TEST_2	LABEL	WORD
		DW	END_MSG_END_TEST_2 - TXT_MSG_END_TEST_2 - 2
		DB	'MAB0112: '
		DB	'Warn with 0 at e'
		DB	'nd'
END_MSG_END_TEST_2		LABEL	WORD
		DB		0
TXT_MSG_END_TEST_3	LABEL	WORD
		DW	END_MSG_END_TEST_3 - TXT_MSG_END_TEST_3 - 2
		DB	'MAB0113: '
		DB	'Err with 0 at en'
		DB	'd'
END_MSG_END_TEST_3		LABEL	WORD
		DB		0
		DW	END_MSG_�� - TXT_MSG_�� - 2
		DB	'MAB0114: '
		DB	'Hlp with 0 at en'
		DB	'd '
END_MSG_��		LABEL	WORD
		DB		0