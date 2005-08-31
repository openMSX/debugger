proc hex2bin { input } {
	set result ""
	foreach {h l} [split $input {}] {
		append result [binary format H2 $h$l] ""
	}
	return $result
}

proc bin2hex { input } {
	set result ""
	foreach i [split $input {}] {
		append result [format %02X [scan $i %c]] ""
	}
	return $result
}
