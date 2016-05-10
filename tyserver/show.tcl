#!/tvbin/tivosh



proc PrintNames {args} {
	set str ""
	set names [lindex $args 0]
	set searchtype 0
	if { [llength $args] > 1 && [info procs "action_search"] == "action_search"} {
	   set searchtype [lindex $args 1]
	   set searchby [lindex "3 4" [expr $searchtype - 1]]
	}
	foreach name $names {
		if { [regexp {(.*)\|(.*)} $name junk last first] } {
			set str1 "$first $last"
			if { $searchtype > 0 } {
				if {$first == ""} {
					set str2 [httpMapReply "$last\t"]
				} else {
					set str2 [httpMapReply "$last, $first\t"]
				}
				set str1 [html_link "/search?searchby=$searchby&q=$str2" $str1]
			}
			if { $str == "" } {
				set str $str1
			} else {
				append str ", $str1"
			}
		}
	}
	return $str
}



proc 	generateLIST		{ } {
  global mfspath list
# puts stdout "generating nowshowing list. \n"
  set list ""
  set entries 0
  set db [dbopen]
  ForeachMfsFile fsid name type "$mfspath" "" {
	RetryTransaction {
		set rec [db $db openid $fsid]

		#Vital data
		set showing [dbobj $rec get Showing]
		set program [dbobj $showing get Program]
		set title [strim [dbobj $program get Title]]
		set episode [strim [dbobj $program get EpisodeTitle]]
		set episodenr [strim [dbobj $program get EpisodeNum]]
		set desc [strim [dbobj $program get Description]]
		set year [dbobj $program get MovieYear]

		#Times
#		set startd [dbobj $rec get StartDate]
#		set stopd [dbobj $rec get StopDate]
#		set expd [dbobj $rec get ExpirationDate]
#		set expt [dbobj $rec get ExpirationTime]
#		set startime [dbobj $rec get StartTime]
#		set stoptime [dbobj $rec get StopTime]
#		set canceldate [dbobj $rec get CancelDate]
#		set ctime [dbobj $rec get CancelTime]
#		set cdate [dbobj $rec get CancelDate]
#		set ddate [dbobj $rec get DeletionDate]
#		set dtime [dbobj $rec get DeletionTime]

		#Airdate
		set  originalairdate [dbobj $program get OriginalAirDate]
		set originalairdatesecs 0
		if { $originalairdate != "" } {
		   set originalairdatesecs [expr $originalairdate * 86400]
		}
		set airdatestr [clock format $originalairdatesecs -format "%1m/%1d/%Y"]


		#Recodring day
		#set seconds [expr [dbobj $showing get Date] * 86400 + [dbobj $showing get Time] + $tzoffset]
		set seconds [expr [dbobj $showing get Date] * 86400 + [dbobj $showing get Time] + 0]
		set day [clock format $seconds -format "%a"]
		set date [clock format $seconds -format "%1m/%1d"]
		set time [clock format $seconds -format "%l:%M %P"]




		#Duration
		set durationsecs 0
		if { $rec != "" } {
		   set stopdate [dbobj $rec get StopDate]
		   set stoptime [dbobj $rec get StopTime]
		   if { $stoptime == "" } {
			  set stoptime [expr ([dbobj $showing get Time] + [dbobj $showing get Duration]) % 86400]
		   }
		   set startdate [dbobj $rec get StartDate]
		   set starttime [dbobj $rec get StartTime]
		   if { $starttime == "" } {
			  set starttime [dbobj $showing get Time]
		   }
		   if { $stopdate != "" } {
			  set durationsecs [expr ($stopdate * 86400 + $stoptime) - ($startdate * 86400 + $starttime)]
		   }
		}
		set showingdurationsecs [dbobj $showing get Duration]
		if { $durationsecs == 0 } {
		   set durationsecs $showingdurationsecs
		}
		set duration [format "%d:%02d" [expr $durationsecs / (60*60)] [expr ($durationsecs % (60*60)) / 60]]
		if { [expr $durationsecs - $showingdurationsecs + 90] < 0 } {
		   append duration [format "/%d:%02d" [expr $showingdurationsecs / (60*60)] [expr ($showingdurationsecs % (60*60)) / 60]]
		}



		set actorstr ""
		set actors [dbobj $program get Actor]
		if { $actors != "" } {
		   set actorstr [PrintNames $actors 1]
		}

		set gueststarstr ""
		set gueststars [dbobj $program get GuestStar]
		if { $gueststars != "" } {
		   set gueststarstr [PrintNames $gueststars 1]
		}

		set hoststr ""
		set hosts [dbobj $program get Host]
		if { $hosts != "" } {
		   set hoststr [PrintNames $hosts 1]
		}

		set directorstr ""
		set directors [dbobj $program get Director]
		if { $directors != "" } {
		   set directorstr [PrintNames $directors 2]
		}

		set execproducerstr ""
		set execproducers [dbobj $program get ExecProducer]
		if { $execproducers != "" } {
		   set execproducerstr [PrintNames $execproducers]
		}

		set producerstr ""
		set producers [dbobj $program get Producer]
		if { $producers != "" } {
		   set producerstr [PrintNames $producers]
		}

		set writerstr ""
		set writers [dbobj $program get Writer]
		if { $writers != "" } {
		   set writerstr [PrintNames $writers]
		}


		set state [dbobj $rec get State]

		set parts [dbobj $rec get Part]

		set fsids ""
		foreach part $parts {
			set file [dbobj $part get File]
			lappend fsids "$file"
		}
		incr entries
	}

	if { $state != 4 } {
		set state 0
	} else {
		set state 1
	}

	set record ""
	append record "StartEntry@ $entries\n"
	append record "RecObjFsid@ $fsid\n"

	set line "FSID@ "
	append line [lindex $fsids 0]
	set fsids [lrange $fsids 1 end]
		foreach part $fsids {
			append line ",$part"
		}
	append record "$line\n"

	append record "State@ $state\n"

	append record "Year@ $year\n"
	append record "AirDate@ $airdatestr\n"
	append record "Day@ $day\n"
	append record "Date@ $date\n"
	append record "Time@ $time\n"
	append record "Duration@ $duration\n"




	append record "Title@ $title: \n"
	append record "Episode@ $episode\n"
	append record "Episidenr@ $episodenr\n"
	append record "Description@ $desc\n"



	append record "Actors@ $actorstr\n"
	append record "GuestStars@ $gueststarstr\n"
	append record "Host@ $hoststr\n"
	append record "Director@ $directorstr\n"
	append record "ExecProd@ $execproducerstr\n"
	append record "Producer@ $producerstr\n"
	append record "Writer@ $writerstr\n"

#        set line "AirDates@ "
#	append line "$startd, $stopd, $expd, $expt, $startime, $stoptime, "
#	append line "$canceldate, $ctime, $cdate, $ddate, $dtime, $oad"
#        append record "$line\n"


#        set line "Actors@ "
#        append line [lindex $actors 0]
#        set fsids [lrange $actors 1 end]
#                foreach part $actors {
#                        append line ",$part"
#                }
#        append record "$line\n"



	append record "EndEntry@ $entries\n"

	append list $record

	};#foreach

	set temp $list
	set list "NumberOfEntries@ $entries\n"
	append list $temp

}


proc strim {str} {
    return [string trim $str "\{\}"]
}


proc versioncheck { } {
global mfspath version
#  puts stdout "checking tivo system sw version"
  set db [dbopen]
  RetryTransaction {
	set version [dbobj [db $db open "/SwSystem/ACTIVE"] get Name]
  }
#  puts -nonewline stdout "it's version $version, setting mfspath to "
  set version [string range $version 0 1]
  if { $version == "2" } { 
	set mfspath "/Recording/NowShowing" 
  } else { 
	set mfspath "/Recording/NowShowingByClassic"
  }
#  puts stdout "$mfspath \n"
}


versioncheck
generateLIST
 
puts stdout $list
