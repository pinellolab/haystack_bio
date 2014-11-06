        var wrap = undefined;//size to display on one line
        var wrap_timer;

        var loadedSequences = new Array();
        //draging details
        var moving_seq;
        var moving_annobox;
        var moving_left;
        var moving_width;
        var moving_both;

        //drag needles
        var dnl = null;
        var dnr = null;
        var drag_is_rc = undefined;
        //container
        var cont = null;

        function rewrap() {
          var previous_wrap = wrap;
          calculate_wrap();
          if (previous_wrap != wrap) {
            for (var seqid in loadedSequences) {
              //exclude inherited properties and undefined properties
              if (!loadedSequences.hasOwnProperty(seqid) || loadedSequences[seqid] === undefined) continue;
            
              var sequence = loadedSequences[seqid];
              var annobox = document.getElementById(seqid + "_annotation");
              var leftPos = parseInt(document.getElementById(seqid + "_dnl").firstChild.firstChild.nodeValue);
              var rightPos = parseInt(document.getElementById(seqid + "_dnr").firstChild.firstChild.nodeValue);
              var is_rc = undefined;
              if (STRAND_HANDLING === 'separate') {
                var fstrand = document.getElementById(seqid + "_fstrand");
                is_rc = !fstrand.checked;
              }
              set_data(leftPos, rightPos - leftPos + 1, is_rc, sequence, annobox);
            }
          }
          if (wrap_timer) {
            clearTimeout(wrap_timer);
          }
          wrap_timer = setTimeout("rewrap()", 5000);
        }

        function show_more(seqid) {
          if (wrap == undefined) rewrap();
          var seq_row = document.getElementById(seqid + "_blocks");
          var info_row = document.getElementById(seqid + "_info");
          var tbl = document.getElementById("tbl_sequences");
          if (info_row) {
            tbl.deleteRow(info_row.rowIndex);
            destroy_seq_handle(seqid, true);
            destroy_seq_handle(seqid, false);
            delete loadedSequences[seqid];
          } else {
            var sequence = new Sequence(seqid, TRANSLATE_DNA, DATABASE_TYPE, STRAND_HANDLING);

            loadedSequences[seqid] = sequence;
            info_row = tbl.insertRow(seq_row.rowIndex + 1);
            info_row.id = seqid + "_info";
            var cell = info_row.insertCell(0);
            cell.colSpan = (STRAND_HANDLING === 'separate' ? 6 : 4);
            cell.style.verticalAlign = "top";
            var width = Math.min(wrap, sequence.length); 
            var start = 1;
            var is_neg_strand = false;
            //center on the first hit
            if (sequence.hits.length > 0) {
              var hit = sequence.hits[0];
              is_neg_strand = hit.is_rc;
              start = Math.max(1, hit.pos + Math.round(hit.width / 2) - Math.round(width / 2));
              if ((start + width - 1) > sequence.length) {
                start = sequence.length - width + 1;
              }
            }
            create_info_section(cell, sequence, seqid, start, width, is_neg_strand);
            var blockCont = document.getElementById(seqid + "_block_container");
            //create sequence view handles
            create_seq_handle(blockCont, seqid, true, start, SEQ_MAX);
            create_seq_handle(blockCont, seqid, false, start + width-1, SEQ_MAX);
          }
        }

        function create_info_section(cell, sequence, seqid, start, width, is_neg_strand) {
          var help;
          var box = document.createElement('div');
          box.className = "box infobox";
          //generic info
          box.appendChild(document.createTextNode("Change the portion of annotated sequence by "));
          var bold_box = document.createElement('b');
          bold_box.appendChild(document.createTextNode("dragging the buttons"));
          box.appendChild(bold_box);
          box.appendChild(document.createTextNode("; hold shift to drag them individually."));
          box.appendChild(document.createElement('br'));
          box.appendChild(document.createElement('br'));
          //description
          var descTitle = document.createElement('h5');
          descTitle.appendChild(document.createTextNode("Comment:"));
          descTitle.className = "inlineTitle";
          box.appendChild(descTitle);
          box.appendChild(document.createTextNode(" " + sequence.desc + " "));
          box.appendChild(help_button("pop_sequence_description"));
          box.appendChild(document.createElement('br'));
          box.appendChild(document.createElement('br'));
          var cpvalueTitle = document.createElement('h5');
          cpvalueTitle.appendChild(document.createTextNode("Combined "));
          var italicp = document.createElement('i');
          italicp.appendChild(document.createTextNode("p"));
          cpvalueTitle.appendChild(italicp);
          cpvalueTitle.appendChild(document.createTextNode("-value:"));
          cpvalueTitle.className = "inlineTitle";
          box.appendChild(cpvalueTitle);
          if (sequence.cpvalue.indexOf("/") != -1) {
            var pos = sequence.cpvalue.indexOf("/");
            var forward = sequence.cpvalue.substring(0, pos);
            var reverse = sequence.cpvalue.substring(pos+1);
            var dim_forward;
            if (forward.indexOf("--") != -1) {
              dim_forward = true;
            } else if (reverse.indexOf("--") != -1) {
              dim_forward = false;
            } else {
              if ( (+forward) > (+reverse)) {
                dim_forward = true;
              } else {
                dim_forward = false;
              }
            }
            var span = document.createElement('span');
            span.className = "dim";
            if (dim_forward) {// dim the forward value
              span.appendChild(document.createTextNode(forward));
              box.appendChild(document.createTextNode(" "));
              box.appendChild(span);
              box.appendChild(document.createTextNode("/"));
              box.appendChild(document.createTextNode(reverse));
            } else {// dim the reverse value
              span.appendChild(document.createTextNode(reverse));
              box.appendChild(document.createTextNode(" "));
              box.appendChild(document.createTextNode(forward));
              box.appendChild(document.createTextNode("/"));
              box.appendChild(span);
            }
          } else {
            box.appendChild(document.createTextNode(" " + sequence.cpvalue));
          }
          box.appendChild(document.createTextNode(" "));
          box.appendChild(help_button("pop_combined_pvalue"));
          
          if (TRANSLATE_DNA === 'y') {
            box.appendChild(document.createElement('br'));
            box.appendChild(document.createElement('br'));
            var bestframeTitle = document.createElement('h5');
            bestframeTitle.appendChild(document.createTextNode("Best frame:"));
            bestframeTitle.className = "inlineTitle";
            box.appendChild(bestframeTitle);
            if (sequence.frame.indexOf("/") != -1) {
              var pos = sequence.frame.indexOf("/");
              var forward = sequence.frame.substring(0, pos);
              var reverse = sequence.frame.substring(pos+1);
              var dim_forward;
              if (forward.indexOf("--") != -1) {
                dim_forward = true;
              } else if (reverse.indexOf("--") != -1) {
                dim_forward = false;
              } else {
                if ( (+forward) > (+reverse)) {
                  dim_forward = true;
                } else {
                  dim_forward = false;
                }
              }
              var span = document.createElement('span');
              span.className = "dim";
              if (dim_forward) {// dim the forward value
                span.appendChild(document.createTextNode(forward));
                box.appendChild(document.createTextNode(" "));
                box.appendChild(span);
                box.appendChild(document.createTextNode("/"));
                box.appendChild(document.createTextNode(reverse));
              } else {// dim the reverse value
                span.appendChild(document.createTextNode(reverse));
                box.appendChild(document.createTextNode(" "));
                box.appendChild(document.createTextNode(forward));
                box.appendChild(document.createTextNode("/"));
                box.appendChild(span);
              }
            } else {
              box.appendChild(document.createTextNode(" " + sequence.frame));
            }
            box.appendChild(document.createTextNode(" "));
            box.appendChild(help_button("pop_frame"));
          }
          box.appendChild(document.createElement('br'));
          box.appendChild(document.createElement('br'));
          //sequence display
          var seqDispTitle = document.createElement('h5');
          seqDispTitle.appendChild(document.createTextNode("Annotated Sequence "));
          seqDispTitle.appendChild(help_button("pop_annotated_sequence"));
          box.appendChild(seqDispTitle);
          var show_rc_only = undefined;
          if (STRAND_HANDLING === 'separate') {
            show_rc_only = is_neg_strand;
            var myform = document.createElement('form');
            var fstrand = document.createElement('input');
            fstrand.id = seqid + "_fstrand";
            fstrand.type = "radio";
            fstrand.name = seqid + "_strand";
            fstrand.onclick = function(evt) {
              handle_strand_change(seqid);
            };
            var rstrand = document.createElement('input');
            rstrand.id = seqid + "_rstrand";
            rstrand.type = "radio";
            rstrand.name = seqid + "_strand";
            rstrand.onclick = function(evt) {
              handle_strand_change(seqid);
            };
            fstrand.checked = !show_rc_only;
            rstrand.checked = show_rc_only;
            myform.appendChild(fstrand);
            myform.appendChild(document.createTextNode("forward strand"));
            myform.appendChild(rstrand);
            myform.appendChild(document.createTextNode("reverse strand"));
            box.appendChild(myform);
          }
          var annobox = document.createElement('div');
          annobox.id = seqid + "_annotation";
          set_data(start, width, show_rc_only, sequence, annobox);
          box.appendChild(annobox);
          cell.appendChild(box);
        }

        function handle_strand_change(seqid) {
          var left = document.getElementById(seqid + "_dnl");
          var right = document.getElementById(seqid + "_dnr");
          var leftPos = parseInt(left.firstChild.firstChild.nodeValue);
          var rightPos = parseInt(right.firstChild.firstChild.nodeValue);
          var fstrand = document.getElementById(seqid + "_fstrand");
          var show_rc_only = !fstrand.checked;
          var data = loadedSequences[seqid];
          if (!data) {
            alert("Sequence not loaded?!");
          }
          var annobox = document.getElementById(seqid + "_annotation");
          set_data(leftPos, rightPos - leftPos + 1, show_rc_only, data, annobox);
        }

        function start_drag(seqid, mouse_on_left, move_both) {
          //first record what we are moving
          moving_left = mouse_on_left;
          moving_both = move_both;
          moving_seq = loadedSequences[seqid];
          if (!moving_seq) {
            alert("Sequence not loaded?!");
          }
          //get the container for movements to be judged against
          cont = document.getElementById(seqid + "_block_container");
          //get the div's that will be moved
          dnl = document.getElementById(seqid + "_dnl");
          dnr = document.getElementById(seqid + "_dnr");
          //get the div which has all the text containers that will be updated
          moving_annobox = document.getElementById(seqid + "_annotation");
          //calculate the space between handles
          moving_width = dnr.firstChild.firstChild.nodeValue - 
          dnl.firstChild.firstChild.nodeValue;
          if (STRAND_HANDLING === 'separate') {
            var fstrand = document.getElementById(seqid + "_fstrand");
            drag_is_rc = !fstrand.checked;
          } else {
            drag_is_rc = undefined;
          }
          //setup the events for draging
          document.onmousemove = handle_drag;
          document.onmouseup = finish_drag;
        }

        function set_data(start, width, show_rc_only, data, annobox) {
          var child = annobox.firstChild;
          var line_width = Math.min(wrap, width);
          var num_per_wrap = (TRANSLATE_DNA === 'y' ? 6 : 5);
          var end = start + width;
          for (var i = start; i < end; i += line_width, line_width = Math.min(wrap, end - i)) {
            for (var j = 0; j < num_per_wrap; ++j) {
              if (child) {
                while (child.firstChild) child.removeChild(child.firstChild);
              } else {
                switch (j) {
                case 0:
                  child = annobox_labels();
                  break;
                case 1:
                  child = annobox_hits();
                  break;
                case 2:
                  child = annobox_matches();
                  break;
                case 3:
                  if (TRANSLATE_DNA === 'y') {
                    child = annobox_translations();
                  } else {
                    child = annobox_sequence();
                  }
                  break;
                case 4:
                  if (TRANSLATE_DNA === 'y') {
                    child = annobox_sequence();
                  } else {
                    child = annobox_boundary();
                  }
                  break;
                case 5:
                  if (TRANSLATE_DNA === 'y') {
                    child = annobox_boundary();
                  }
                  break;
                }
                annobox.appendChild(child);
              }
              switch (j) {
              case 0:
                data.append_labels(child, i, line_width, show_rc_only);
                break;
              case 1:
                data.append_hits(child, i, line_width, show_rc_only);
                break;
              case 2:
                data.append_matches(child, i, line_width, show_rc_only);
                break;
              case 3:
                if (TRANSLATE_DNA === 'y') {
                  data.append_translation(child, i, line_width, show_rc_only);
                } else {
                  data.append_seq(child, i, line_width, show_rc_only);
                }
                break;
              case 4:
                if (TRANSLATE_DNA === 'y') {
                  data.append_seq(child, i, line_width, show_rc_only);
                } else {
                  data.append_boundary(child, i, line_width, show_rc_only);
                }
                break;
              case 5:
                if (TRANSLATE_DNA === 'y') {
                  data.append_boundary(child, i, line_width, show_rc_only);
                }
                break;
              }
              child = child.nextSibling;
            }
          }
          //clean up excess
          
          while (child) {
            var next = child.nextSibling;
            annobox.removeChild(child);
            child = next;
          }
        }
        
        function mouseCoords(ev) {
          ev = ev || window.event;
          if(ev.pageX || ev.pageY){ 
	          return {x:ev.pageX, y:ev.pageY}; 
	        } 
	        return { 
	          x:ev.clientX + document.body.scrollLeft - document.body.clientLeft, 
	          y:ev.clientY + document.body.scrollTop  - document.body.clientTop 
	        };
        }

        function setup() {
          rewrap();
          window.onresize = delayed_rewrap;
        }

        function calculate_wrap() {
          var est_wrap = 0;
          var page_width;
          if (window.innerWidth) {
            page_width = window.innerWidth;
          } else if (document.body) {
            page_width = document.body.clientWidth;
          } else {
            page_width = null;
          }
          if (page_width) {
            var ruler_width = document.getElementById("ruler").offsetWidth;
            est_wrap = Math.floor(page_width / ruler_width) * 5;
          }
          if (est_wrap > 20) {
            wrap = est_wrap - 20; 
          } else {
            wrap = 100;
          }
        }


        function delayed_rewrap() {
          if (wrap_timer) {
            clearTimeout(wrap_timer);
          }
          wrap_timer = setTimeout("rewrap()", 1000);
        }

        function showHidden(prefix) {
          document.getElementById(prefix + '_activator').style.display = 'none';
          document.getElementById(prefix + '_deactivator').style.display = 'block';
          document.getElementById(prefix + '_data').style.display = 'block';
        }
        function hideShown(prefix) {
          document.getElementById(prefix + '_activator').style.display = 'block';
          document.getElementById(prefix + '_deactivator').style.display = 'none';
          document.getElementById(prefix + '_data').style.display = 'none';
        }




        function annobox_labels() {
          var seqDispLabel = document.createElement('div');
          seqDispLabel.className = "sequence sequence_labels";
          seqDispLabel.style.height = "2.5em";
          return seqDispLabel;
        }

        function annobox_hits() {
          var seqDispHit = document.createElement('div');
          seqDispHit.className = "sequence";
          seqDispHit.style.height = "1.5em";
          return seqDispHit;
        }

        function annobox_matches() {
          var seqDispMatch = document.createElement('div');
          seqDispMatch.className = "sequence";
          seqDispMatch.style.height = "1.5em";
          return seqDispMatch;
        }

        function annobox_translations() {
          var seqDispXlate = document.createElement('div');
          seqDispXlate.className = "sequence";
          seqDispXlate.style.height = "1.5em";
          return seqDispXlate;
        }

        function annobox_sequence() {
          var seqDispSeq = document.createElement('div');
          seqDispSeq.className = "sequence";
          seqDispSeq.style.height = "1.5em";
          return seqDispSeq; 
        }
        function annobox_boundary() {
          var seqDispSeq = document.createElement('div');
          seqDispSeq.className = "sequence";
          seqDispSeq.style.height = "1.5em";
          return seqDispSeq; 
        }

        function create_seq_handle(container, seqid, isleft, pos, max) {
          var vbar = document.createElement('div');
          vbar.id = seqid + "_dn" + (isleft ? "l" : "r");
          vbar.className = "block_needle";
          //the needles sit between the sequence positions, so the left one sits at the start and the right at the end
          //this is why 1 is subtracted off the position for a left handle
          vbar.style.left = "" + ((isleft ? (pos - 1) : pos) / max * 100)+ "%";
          var label = document.createElement('div');
          label.className = "block_handle";
          label.appendChild(document.createTextNode("" + pos));
          label.style.cursor = 'pointer';
          label.title = "Drag to move the displayed range. Hold shift and drag to change " + (isleft ? "lower" : "upper") + " bound of the range.";
          vbar.appendChild(label);
          container.appendChild(vbar);
          label.onmousedown = function(evt) {
            evt = evt || window.event;
            start_drag(seqid, isleft, !(evt.shiftKey));
          };
        }

        function destroy_seq_handle(seqid, isleft) {
          var vbar = document.getElementById(seqid + "_dn" + (isleft ? "l" : "r"));
          var label = vbar.firstChild;
          label.onmousedown = null;
          vbar.parentNode.removeChild(vbar);
        }




        function calculate_width(obj) {
          return obj.clientWidth - (obj.style.paddingLeft ? obj.style.paddingLeft : 0) 
              - (obj.style.paddingRight ? obj.style.paddingRight : 0);
        }

        function calculate_drag_pos(ev) {
          var mouse = mouseCoords(ev);
          var dragable_length = calculate_width(cont);
          //I believe that the offset parent is the body
          //otherwise I would need to make this recursive
          //maybe clientLeft would work, but the explanation of
          //it is hard to understand and it apparently doesn't work
          //in firefox 2.
          var diff = mouse.x - cont.offsetLeft;
          if (diff < 0) diff = 0;
          if (diff > dragable_length) diff = dragable_length;
          var pos = Math.round(diff / dragable_length * (SEQ_MAX));
          return pos + 1;
        }

        function handle_drag(ev) {
          var pos = calculate_drag_pos(ev);
          update_needles(pos, moving_seq.length, false);
        }

        function finish_drag(ev) {
          document.onmousemove = null;
          document.onmouseup = null;
          var pos = calculate_drag_pos(ev);
          update_needles(pos, moving_seq.length, true);
        }

        function update_needles(pos, seqlen, updateTxt) {
          var leftPos = parseInt(dnl.firstChild.firstChild.nodeValue);
          var rightPos = parseInt(dnr.firstChild.firstChild.nodeValue);
          if (moving_both) {
            if (moving_left) {
              if ((pos + moving_width) > seqlen) {
                pos = seqlen - moving_width;
              }
              leftPos = pos;
              rightPos = pos + moving_width;
            } else {
              if (pos > seqlen) {
                pos = seqlen;
              } else if ((pos - moving_width) < 1) {
                pos = moving_width + 1;
              }
              leftPos = pos - moving_width;
              rightPos = pos;
            }
          } else {
            if (moving_left) {
              if (pos > rightPos) {
                pos = rightPos;
              }
              leftPos = pos;
            } else {
              if (pos < leftPos) {
                pos = leftPos;
              } else if (pos > seqlen) {
                pos = seqlen;
              }
              rightPos = pos;
            }
          }
          set_needles(SEQ_MAX, dnl, leftPos, dnr, rightPos);
          if (updateTxt) set_data(leftPos, rightPos - leftPos + 1, drag_is_rc, moving_seq, moving_annobox);
        }

        function set_needles(max, left, leftPos, right, rightPos) {
          //the needles are between the sequence positions
          //the left needle is before and the right needle after
          //think of it as an inclusive range...
          left.style.left = "" + ((leftPos -1) / max * 100)+ "%";
          left.firstChild.firstChild.nodeValue="" + leftPos;
          right.style.left = "" + (rightPos / max * 100)+ "%";
          right.firstChild.firstChild.nodeValue="" + rightPos;
        }


        function append_coloured_nucleotide_sequence(container, sequence) {
          var len = sequence.length;
          for (var i = 0; i < len; i++) {
            var colour = "black";
            switch (sequence.charAt(i)) {
            case 'A':
              colour = "red";
              break;
            case 'C':
              colour = "blue";
              break;
            case 'G':
              colour = "orange";
              break;
            case 'T':
              colour = "green";
              break;
            }
            var letter = document.createElement('span');
            letter.style.color = colour;
            letter.appendChild(document.createTextNode(sequence.charAt(i)));
            container.appendChild(letter);
          }
        }

        function append_coloured_peptide_sequence(container, sequence) {
          var len = sequence.length;
          for (var i = 0; i < len; i++) {
            var colour = "black";
            switch (sequence.charAt(i)) {
            case 'A':
            case 'C':
            case 'F':
            case 'I':
            case 'L':
            case 'V':
            case 'W':
            case 'M':
              colour = "blue";
              break;
            case 'N':
            case 'Q':
            case 'S':
            case 'T':
              colour = "green";
              break;
            case 'D':
            case 'E':
              colour = "magenta";
              break;
            case 'K':
            case 'R':
              colour = "red";
              break;
            case 'H':
              colour = "pink";
              break;
            case 'G':
              colour = "orange";
              break;
            case 'P':
              colour = "yellow";
              break;
            case 'Y':
              colour = "turquoise";
              break;
            }
            var letter = document.createElement('span');
            letter.style.color = colour;
            letter.appendChild(document.createTextNode(sequence.charAt(i)));
            container.appendChild(letter);
          }
        }

        /*
         * Finds the index of an item in a sorted array (a) that equals a key (k)
         * when compared using a comparator (c). If an exact match can not be found
         * then returns -(index+1), where index is the location that the item would be inserted.
         */
        function bsearch(a, c, k) {
          var low = 0;
          var high = a.length;
          while (low < high) {
            var mid = low + Math.floor((high - low) /2);
            if (c(a[mid], k) < 0) {
              low = mid + 1;
            } else {
              high = mid;
            }
          }
          if ((low < a.length) && (c(a[low], k) == 0)) {
            return low;
          } else {
            return -(low+1);
          }
        }

        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // Start Motif Object
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        function Motif(name, type, best_f, best_r) {
          this.name = name;
          this.is_nucleotide = (type === "nucleotide");
          this.best_f = best_f;
          if (best_r.length == 0) {
            var txt = best_f.split("");
            txt.reverse();
            best_r = txt.join("");
          }
          this.best_r = best_r;
          this.length = best_f.length;
        }
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // End Motif Object
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // Start Seg Object
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        function Seg(start, segment) {
          this.start = parseInt(start);
          this.segment = segment;
        }

        function compare_seg_to_pos(seg, pos) {
          return seg.start - pos;
        }
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // End Seg Object
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // Start Hit Object
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        function Hit(sequence_is_nucleotide, motif, pos, strand, pvalue, match, xlated) {
          //properties
          this.motif = motif;
          this.pos = parseInt(pos);
          this.width = 0;
          this.is_rc = (strand != "+");
          this.pvalue = pvalue;
          this.xlated = "";
          this.best = "";
          this.match = "";
          //functions
          this.find_overlap = Hit_find_overlap;
          //setup
          var seq;
          var m = match.replace(/ /g, "\u00A0");
          if (this.is_rc) seq = this.motif.best_r;
          else seq = this.motif.best_f;
          if (sequence_is_nucleotide == this.motif.is_nucleotide) {
            this.best = seq;
            this.match = m;
            this.xlated = xlated;
            this.width = motif.length;
          } else if (sequence_is_nucleotide) {
            this.width = motif.length * 3;
            for (var i = 0; i < motif.length; i++) {
              this.best += seq.charAt(i);
              this.best += "..";
              this.xlated += xlated.charAt(i);
              this.xlated += "..";
              this.match += m.charAt(i);
              this.match += "\u00A0\u00A0";
            }
          } else {
            throw "UNSUPPORTED_CONVERSION";
          }
        }

        function Hit_find_overlap(sequence_is_nucleotide, start, width) {
          if (this.pos < start) {
            if ((this.pos + this.width) > start) {
              return {hit: this, start: start, length: Math.min(width, (this.pos + this.width - start))};
            } else {
              return null;
            }
          } else if (this.pos < (start + width)) {
            return {hit: this, start: this.pos, length: Math.min(this.width, (start + width - this.pos))};
          }
          return null;
        }

        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // End Hit Object
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // Start Overlapping Hit Iterator
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        function OLHIterator(sequence, start, width, is_rc) {
          //properties
          this.sequence = sequence;
          this.start = start;
          this.width = width;
          this.is_rc = is_rc;
          this.both = (is_rc === undefined);
          this.index = 0;
          this.nextO = null;
          //methods
          this.next = OLHIterator_next;
          this.has_next = OLHIterator_has_next;
          //setup
          // find the first hit which overlaps the range
          for (; this.index < this.sequence.hits.length; this.index++) {
            var hit = this.sequence.hits[this.index];
            if (!this.both && this.is_rc != hit.is_rc) continue;
            if ((this.nextO = hit.find_overlap(this.sequence.is_nucleotide, this.start, this.width)) != null) break;
          }
        }
        
        function OLHIterator_next() {
          if (this.nextO == null) throw "NO_NEXT_ELEMENT";
          var current = this.nextO;
          this.nextO = null;
          for (this.index = this.index + 1; this.index < this.sequence.hits.length; this.index++) {
            var hit = this.sequence.hits[this.index];
            if (!this.both && this.is_rc != hit.is_rc) continue;
            this.nextO = hit.find_overlap(this.sequence.is_nucleotide, this.start, this.width);
            break;
          }
          return current;
        }
        
        function OLHIterator_has_next() {
          return (this.nextO != null);
        }

        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // End Overlapping Hit Iterator
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // Start Sequence Object
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        function Sequence(seqid, translate_dna, database_type, strand_handling) {
          //properties
          this.has_frame = (translate_dna === 'y');
          this.db_dna = (database_type === 'nucleotide');
          this.has_strand = (this.db_dna && strand_handling !== 'separate');
          this.length = parseInt(document.getElementById(seqid + "_len").value);
          this.desc = document.getElementById(seqid + "_desc").value;
          this.cpvalue = document.getElementById(seqid + "_combined_pvalue").value;
          if (this.has_frame) {
            this.frame = document.getElementById(seqid + "_frame").value;
          }
          this.is_nucleotide = (document.getElementById(seqid + "_type").value === "nucleotide");
          this.segs = new Array(); //sorted list of segs
          this.hits = new Array(); //sorted list of hits
          //functions
          this.get_overlapping_hits = Sequence_get_overlapping_hits;
          this.append_seq = Sequence_append_seq;
          this.append_translation = Sequence_append_translation;
          this.append_hits = Sequence_append_hits;
          this.append_matches = Sequence_append_matches;
          this.append_labels = Sequence_append_labels;
          this.append_boundary = Sequence_append_boundary;
          //init
          //made this parser much more permissive as a new
          //version of libxml2 broke the translate call I was
          //using to remove whitespace characters
          var mysegs = document.getElementById(seqid + "_segs");
          var tokens = mysegs.value.split(/\s+/);
          var offsetnum = "";
          var seqchunk = "";
          for (var i = 0; i < tokens.length; i++) {
            var token = tokens[i];
            if (token == "") continue;
            if (token.match(/\d+/)) {
              if (offsetnum != "" && seqchunk != "") {
                this.segs.push(new Seg(offsetnum, seqchunk));
              }
              offsetnum = parseInt(token, 10);
              seqchunk = "";
              continue;
            }
            seqchunk += token;
          }
          if (offsetnum != "" && seqchunk != "") {
            this.segs.push(new Seg(offsetnum, seqchunk));
          }
          var myhits = document.getElementById(seqid + "_hits");
          lines = myhits.value.split(/\n/);
          for (var i in lines) {
            //exclude inherited properties and undefined properties
            if (!lines.hasOwnProperty(i) || lines[i] === undefined) continue;
            
            var line = lines[i];
            var chunks = line.split(/\t/);
            if (chunks.length != 6) continue;
            var pos = chunks[0];
            var motif = motifs[chunks[1]];
            var strand = chunks[2];
            var pvalue = chunks[3];
            var match = chunks[4];
            var xlated = chunks[5];
            this.hits.push(new Hit(this.is_nucleotide, motif, pos, strand, pvalue, match, xlated));
          }
        }

        function Sequence_get_overlapping_hits(start, width, is_rc) {
          return new OLHIterator(this, start, width, is_rc);
        }

        function Sequence_append_seq(container, start, width, is_rc) {
          if (start > this.length) {
            alert("start: " + start + " length: " + this.length);
            throw "INDEX_OUT_OF_BOUNDS";
          }
          if ((start + width - 1) > this.length) {
            alert("start: " + start + " width: " + width + " length: " + this.length);
            throw "RANGE_OUT_OF_BOUNDS";
          }
          //make a sub container to put the sequence in
          var mycontainer = document.createElement('span');
          // find where a seg starting at start would be
          var i = bsearch(this.segs, compare_seg_to_pos, start);
          var seg;
          var sequence = "";
          if (i < -1) {
            //possible partial segment, need to handle first
            i = -(i + 1);
            seg = this.segs[i-1];
            if ((seg.start + seg.segment.length) > start) {
              var seg_offset = start - seg.start;
              var seg_width = Math.min(width, seg.segment.length - seg_offset);
              sequence += seg.segment.substring(seg_offset, seg_offset + seg_width);
            }
          } else if (i == -1) {
            //gap, with following segment, normal state at start of iteration
            i = 0;
          } 
          for (;i < this.segs.length; i++) {
            seg = this.segs[i];
            var gap_width = Math.min(width - sequence.length, seg.start - (start + sequence.length));
            for (; gap_width > 0; gap_width--) sequence += '-';
            var seg_width = Math.min(width - sequence.length, seg.segment.length);
            if (seg_width == 0) break;
            sequence += seg.segment.substring(0, seg_width);
          }
          while (sequence.length < width) sequence += '-';
          // calculate which parts are aligned with hits and output them in colour
          var pos = start;
          var iter = this.get_overlapping_hits(start, width, is_rc);
          while (iter.has_next()) {
            var o = iter.next();
            var from, to;
            if (o.start > pos) {
              from = pos - start;
              to = o.start - start;
              var subseq = sequence.substring(from, to);
              var greytxt = document.createElement('span');
              greytxt.style.color = 'grey';
              greytxt.appendChild(document.createTextNode(subseq));
              mycontainer.appendChild(greytxt);
              pos = o.start;
            }
            from = pos - start;
            to = from + o.length;
            var motifseq = sequence.substring(from, to);
            if (this.is_nucleotide) {
              append_coloured_nucleotide_sequence(mycontainer, motifseq);
            } else {
              append_coloured_peptide_sequence(mycontainer, motifseq);
            }
            pos += o.length;
          }
          if (pos < (start + width)) {
            var greytxt = document.createElement('span');
            greytxt.style.color = 'grey';
            greytxt.appendChild(document.createTextNode(sequence.substring(pos - start)));
            mycontainer.appendChild(greytxt);
          }
          container.appendChild(mycontainer);
        }

        function Sequence_append_translation(container, start, width, is_rc) {
          if (start > this.length) {
            alert("start: " + start + " length: " + this.length);
            throw "INDEX_OUT_OF_BOUNDS";
          }
          if ((start + width - 1) > this.length) {
            alert("start: " + start + " width: " + width + " length: " + this.length);
            throw "RANGE_OUT_OF_BOUNDS";
          }
          //make a sub container to put the sequence in
          var mycontainer = document.createElement('span');
          // calculate which parts are aligned with hits and output them in colour
          var pos = start;
          var iter = this.get_overlapping_hits(start, width, is_rc);
          while (iter.has_next()) {
            var o = iter.next();
            var space = "";
            var from, to;
            while (o.start > pos) {
              space += "\u00A0";
              ++pos;
            }
            if (space.length > 0) {
              var spacer = document.createElement('span');
              spacer.appendChild(document.createTextNode(space));
              mycontainer.appendChild(spacer);
            }
            from = o.start - o.hit.pos;
            to = from + o.length;
            var motifseq = o.hit.xlated.substring(from, to);
            if (o.hit.motif.is_nucleotide) {
              append_coloured_nucleotide_sequence(mycontainer, motifseq);
            } else {
              append_coloured_peptide_sequence(mycontainer, motifseq);
            }
            pos += o.length;
          }

          container.appendChild(mycontainer);
        }

        function Sequence_append_matches(container, start, width, is_rc) {
          if (start > this.length) {
            alert("start: " + start + " length: " + this.length);
            throw "INDEX_OUT_OF_BOUNDS";
          }
          if ((start + width - 1) > this.length) {
            alert("start: " + start + " width: " + width + " length: " + this.length);
            throw "RANGE_OUT_OF_BOUNDS";
          }
          //make a sub container to put the sequence in
          var mycontainer = document.createElement('span');
          var pos = start;
          var text = "";
          var iter = this.get_overlapping_hits(start, width, is_rc);
          while (iter.has_next()) {
            var o = iter.next();
            var space = "";
            var from, to;
            while (o.start > pos) {
              text += "\u00A0";
              ++pos;
            }
            from = o.start - o.hit.pos;
            to = from + o.length;
            var motifseq = o.hit.match.substring(from, to);
            text += motifseq
            pos += o.length;
          }
          mycontainer.appendChild(document.createTextNode(text));
          container.appendChild(mycontainer);
        }

        function Sequence_append_hits(container, start, width, is_rc) {
          if (start > this.length) {
            alert("start: " + start + " length: " + this.length);
            throw "INDEX_OUT_OF_BOUNDS";
          }
          if ((start + width - 1) > this.length) {
            alert("start: " + start + " width: " + width + " length: " + this.length);
            throw "RANGE_OUT_OF_BOUNDS";
          }
          //make a sub container to put the sequence in
          var mycontainer = document.createElement('span');
          // calculate which parts are aligned with hits and output them in colour
          var pos = start;
          var iter = this.get_overlapping_hits(start, width, is_rc);
          while (iter.has_next()) {
            var o = iter.next();
            var space = "";
            var from, to;
            while (o.start > pos) {
              space += "\u00A0";
              ++pos;
            }
            if (space.length > 0) {
              var spacer = document.createElement('span');
              spacer.appendChild(document.createTextNode(space));
              mycontainer.appendChild(spacer);
            }
            from = o.start - o.hit.pos;
            to = from + o.length;
            var motifseq = o.hit.best.substring(from, to);
            if (o.hit.motif.is_nucleotide) {
              append_coloured_nucleotide_sequence(mycontainer, motifseq);
            } else {
              append_coloured_peptide_sequence(mycontainer, motifseq);
            }
            pos += o.length;
          }

          container.appendChild(mycontainer);
        }

        function Sequence_append_labels(container, start, width, is_rc) {
          if (start > this.length) {
            alert("start: " + start + " length: " + this.length);
            throw "INDEX_OUT_OF_BOUNDS";
          }
          if ((start + width - 1) > this.length) {
            alert("start: " + start + " width: " + width + " length: " + this.length);
            throw "RANGE_OUT_OF_BOUNDS";
          }
          //create a table (ye gods...)
          var oTable = document.createElement("table");
          var oRow = oTable.insertRow(oTable.rows.length);

          // calculate where to put the labels
          var pos = start;
          var cellindex = 0;
          var iter = this.get_overlapping_hits(start, width, is_rc);
          while (iter.has_next()) {
            var o = iter.next();
            var motif_center = Math.floor(o.start + o.length / 2);
            var spacer = "";
            while (pos < motif_center) {
              spacer += "\u00A0";
              pos++;
            }
            var cell1 = oRow.insertCell(cellindex++);
            cell1.appendChild(document.createTextNode(spacer));
            //add one for the div
            pos++;
            var cell2 = oRow.insertCell(cellindex++);
            //create the div that holds the label divs
            var div_container = document.createElement('div');
            div_container.className = "label_container";
            cell2.appendChild(div_container);
            //create the top label
            var top_label = "";
            if (this.has_strand || this.has_frame) {
              top_label += "(";
              if (this.has_strand) {
                if (o.hit.is_rc) {
                  top_label += "-";
                } else {
                  top_label += "+";
                }
              }
              if (this.has_frame) {
                switch ((o.hit.pos - 1) % 3) {
                  case 0:
                  top_label += "a";
                  break;
                  case 1:
                  top_label += "b";
                  break;
                  default:
                  top_label += "c";
                  break;
                }
              }
              top_label += ") ";
            }
            if (o.hit.motif.name.match(/^\d+$/)) {
              top_label += "Motif " + o.hit.motif.name;
            } else {
              top_label += o.hit.motif.name;
            }
            var div_top_label = document.createElement('div');
            div_top_label.className = "sequence_label sequence_label_top";
            div_top_label.style.width = "" + top_label.length + "em";
            div_top_label.style.left = "-" + (top_label.length / 2) + "em";
            div_top_label.appendChild(document.createTextNode(top_label));
            //create the bottom label
            var bottom_label = o.hit.pvalue;
            var div_bottom_label = document.createElement('div');
            div_bottom_label.className = "sequence_label sequence_label_bottom";
            div_bottom_label.style.width = "" + bottom_label.length + "em";
            div_bottom_label.style.left = "-" + (bottom_label.length / 2) + "em";
            div_bottom_label.appendChild(document.createTextNode(bottom_label));
            //add the label divs to the positioned container
            div_container.appendChild(div_top_label);
            div_container.appendChild(div_bottom_label);
          }
          container.appendChild(oTable);

        }

        function Sequence_append_boundary(container, start, width, is_rc) {
          if (start > this.length) {
            alert("start: " + start + " length: " + this.length);
            throw "INDEX_OUT_OF_BOUNDS";
          }
          if ((start + width - 1) > this.length) {
            alert("start: " + start + " width: " + width + " length: " + this.length);
            throw "RANGE_OUT_OF_BOUNDS";
          }
          //make a sub container to put the sequence in
          var mycontainer = document.createElement('span');
          var pos = start;
          var text = "";
          var iter = this.get_overlapping_hits(start, width, is_rc);
          while (iter.has_next()) {
            var o = iter.next();
            var end;
            while (o.start > pos) {
              text += "\u00A0";
              ++pos;
            }
            if (o.start == o.hit.pos) {
              text += "\\";
              ++pos;
            }
            end = o.start + o.length - 1;
            while (end > pos) {
              text += "_";
              ++pos;
            }
            if (end == (o.hit.pos + o.hit.width -1)) {
              text += "/"
            } else {
              text += "_"
            }
            ++pos;
          }
          mycontainer.appendChild(document.createTextNode(text));
          container.appendChild(mycontainer);
        }
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // End Sequence Object
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
