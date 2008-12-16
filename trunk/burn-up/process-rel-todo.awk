BEGIN { pre_sum = 0; post_sum = 0; add_to_pre_sum = 1; } 

/^------/ { add_to_pre_sum = 0; } 

{ for (i=1; i<NF; i++) {
	if ($i == ":::") {
	    if (add_to_pre_sum) {
		pre_sum += $(i+1)+0
	    } else { 
		post_sum += $(i+1)+0
	    }
	}
    }
}

END {
    x = system("date +%s > proc-rel.tmp")
    getline < "proc-rel.tmp";
    n_s = $1-1229420879;
    n_days = n_s/(60*60*24)
    print n_days, post_sum, pre_sum + post_sum
}
	

