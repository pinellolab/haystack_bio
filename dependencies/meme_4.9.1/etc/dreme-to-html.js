      var expansion_lookup = [];

      /*
       * show_hidden
       *
       * Looks for specially named elements and switches to the shown view
       */
      function show_hidden(prefix) {
        document.getElementById(prefix + '_activator').style.display = 'none';
        document.getElementById(prefix + '_deactivator').style.display = 'block';
        document.getElementById(prefix + '_data').style.display = 'block';
      }
      /*
       * hide_shown
       *
       * Looks for specially named elements and switches to the hidden view
       */
      function hide_shown(prefix) {
        document.getElementById(prefix + '_activator').style.display = 'block';
        document.getElementById(prefix + '_deactivator').style.display = 'none';
        document.getElementById(prefix + '_data').style.display = 'none';
      }

      function click_download_tab(tab) {
        document.getElementById("download_tab_num").value = tab;
        for (var i = 1; i <= 3; i++) {
          document.getElementById('download_tab_'+i).className = "tab" + (i==tab ? " activeTab" : "");
          document.getElementById('download_pnl_'+i).style.display = (i==tab ? "block" : "none");
        }
        document.getElementById('download_button').style.visibility = (tab==3 ? "visible" : "hidden");
      }


      /*
       * searches child nodes in depth first order and returns the
       * first it finds with the className specified.
       */
      function find_child_element_by_class(node, className) {
        var patt = new RegExp("\\b" + className + "\\b");

        if (node.nodeType == Node.ELEMENT_NODE && 
            patt.test(node.className)) {
          return node;
        } else {
          var result = null;
          for (var i = 0; i < node.childNodes.length; i++) {
            result = find_child_element_by_class(node.childNodes[i], className);
            if (result != null) break;
          }
          return result;
        }
      }

      function find_parent_element_by_class(node, className) {
        var patt = new RegExp("\\b" + className + "\\b");
        if (node.nodeType == Node.ELEMENT_NODE && 
            patt.test(node.className)) {
          return node;
        } else if (node.parentNode != null) {
          return find_parent_element_by_class(node.parentNode, className);
        }
        return null;
      }

      /*
       * expand
       *
       * Expand the extra data section for a motif.
       */
      function expand(num) {
        // get motif data
        var motif_info = motif_seqs[num];
        var motif_id = motif_info[0];
        var seq = motif_info[1];
        var rcseq = motif_info[2];
        var length = motif_info[3];
        var nsites = motif_info[4];
        var p_hits = motif_info[5];
        var n_hits = motif_info[6];
        var pvalue = motif_info[7];
        var evalue = motif_info[8];
        var uevalue = motif_info[9];
        var matches = motif_info[10];
        // find the location to insert the expanded motif data
        var table = document.getElementById('dreme_motifs');
        var motif_row = document.getElementById('motif_row_' + num);
        var exp_row = table.insertRow(motif_row.rowIndex + 1);
        exp_row.id = 'exp_row_' + num;
        var cell = exp_row.insertCell(0);
        cell.colSpan = 9;
        // create the DOM to insert
        var exp = document.getElementById('expanded_motif').firstChild.cloneNode(true);
        // update fields
        set_content_to_text(find_child_element_by_class(exp, 'name'), seq);
        set_content_to_text(find_child_element_by_class(exp, 'num'), num);
        var img = find_child_element_by_class(exp, 'img_nc');
        img.src = motif_id + "nc_" + seq + ".png";
        var imgrc = find_child_element_by_class(exp, 'img_rc');
        imgrc.src = motif_id + "rc_" + rcseq + ".png";
        // fill in the details
        var details = find_child_element_by_class(exp, 'details');
        set_content_to_text(find_child_element_by_class(details, 'positives'), p_hits);
        set_content_to_text(find_child_element_by_class(details, 'negatives'), n_hits);
        set_content_to_text(find_child_element_by_class(details, 'pvalue'), pvalue);
        set_content_to_text(find_child_element_by_class(details, 'evalue'), evalue);
        set_content_to_text(find_child_element_by_class(details, 'uevalue'), uevalue);
        
        // fill in match table
        var match_row = find_child_element_by_class(exp, 'match');
        var tbody = match_row.parentNode;
        for (var i = 0; i < matches.length; i++) {
          var match = matches[i];
          var cseq = match[0];
          var cpos = match[1];
          var cneg = match[2];
          var cpval = match[3].toExponential(1);
          var ceval = match[4].toExponential(1);
          var row = match_row.cloneNode(true);
          var td_cseq = find_child_element_by_class(row, 'dnaseq');
          set_content_to_text(td_cseq, cseq);
          colour_dna_seq(td_cseq);
          set_content_to_text(find_child_element_by_class(row, 'positives'), cpos);
          set_content_to_text(find_child_element_by_class(row, 'negatives'), cneg);
          set_content_to_text(find_child_element_by_class(row, 'pvalue'), cpval);
          set_content_to_text(find_child_element_by_class(row, 'evalue'), ceval);
          tbody.appendChild(row);
        }
        tbody.removeChild(match_row);
        // append the expanded information
        cell.appendChild(exp);
        // hide the old row
        motif_row.style.display = 'none';
        update_headers();
      }

      function expanded_num(elem) {
        var exp = find_parent_element_by_class(elem, 'expanded_motif');
        var num = parseInt(nodes_text(text_nodes(find_child_element_by_class(exp, 'num'))));
        return num;
      }

      function contract(contained_node) {
        var table = document.getElementById('dreme_motifs');
        var num = expanded_num(contained_node);
        var motif_row = document.getElementById('motif_row_' + num);
        var exp_row = document.getElementById('exp_row_' + num);

        motif_row.style.display = 'table-row';
        table.deleteRow(exp_row.rowIndex);
        update_headers();
      }

      function update_headers() {
        var motif_row_patt = new RegExp("\\bmotif_row\\b");
        var motif_head_patt = new RegExp("\\bmotif_head\\b");
        var table = document.getElementById('dreme_motifs');
        var header = table.tHead.getElementsByTagName('tr')[0];
        header.style.display = 'none';
        var trs = table.tBodies[0].getElementsByTagName('tr');
        var needHeader = true;
        for (var i = 0; i < trs.length; i++) {
          var row = trs[i];
          if (row.style.display == 'none') continue;
          if (motif_row_patt.test(row.className)) {
            if (needHeader) {
              var dupHeader = header.cloneNode(true);
              dupHeader.style.display = 'table-row';
              row.parentNode.insertBefore(dupHeader, row);
              needHeader = false;
              i++;
            }
          } else if (motif_head_patt.test(row.className)) {
            table.deleteRow(row.rowIndex);
            i--;
          } else {
            needHeader = true;
          }
        }
      }

      function set_content_to_text(ele, text) {
        while(ele.hasChildNodes()) {
          ele.removeChild(ele.firstChild);
        }
        ele.appendChild(document.createTextNode(text));
      }

      function both_setup(pos) {
        // get the total number of motifs
        var nmotifs = parseInt(document.getElementById('nmotifs').value, 10);
        // set the motif that we're submitting
        document.getElementById('submit_motif').value = (pos == 0 ? 'all' : pos);
        document.getElementById('send_to_selector').style.display = (pos == 0 ? 'none' : 'block');
        document.getElementById('send_to_title_1').style.display = (pos == 0 ? 'none' : 'block');
        document.getElementById('send_to_title_2').style.display = (pos == 0 ? 'block' : 'none');

        if (pos != 0) {
          // get the information for the position
          var motif_seq = motif_seqs[pos];
          var motif_id = motif_seq[0];
          var seq = motif_seq[1];
          var rcseq = motif_seq[2];
          // set the motif number
          // set the titles of both popups
          set_content_to_text(document.getElementById('send_to_name'), seq);
          set_content_to_text(document.getElementById('download_name'), seq);
          // set the images
          var nc_img = "" + motif_id + "nc_" + seq + ".png";
          var rc_img = "" + motif_id + "rc_" + rcseq + ".png";
          var img;
          img = document.getElementById('send_to_img');
          img.src = nc_img;
          img.style.display = "inline";
          img = document.getElementById('send_to_rcimg');
          img.src = rc_img;
          img.style.display = "inline";
          img = document.getElementById('download_img');
          img.src = nc_img;
          img.style.display = "inline";
          img = document.getElementById('download_rcimg');
          img.src = rc_img;
          img.style.display = "inline";
          // hide the canvas
          document.getElementById('send_to_can').style.display = "none";
          document.getElementById('send_to_rccan').style.display = "none";
          document.getElementById('download_can').style.display = "none";
          document.getElementById('download_rccan').style.display = "none";
          // get some motif details
          var pspm_text = document.getElementById("pspm"+ pos).value;
          var pspm = new Pspm(pspm_text);
          var alpha = new Alphabet(document.getElementById("alphabet").value, 
              document.getElementById("bgfreq").value);
          document.getElementById('download_pspm').value = pspm.as_pspm();
          document.getElementById('download_pssm').value = pspm.as_pssm(alpha);
          // set the width and height defaults
          document.getElementById('logo_width').value = pspm.get_motif_length();
          document.getElementById('logo_height').value = 7.5;
          // hide and show the arrows
          var prevclass = (pos == 1 ? "navarrow inactive" : "navarrow");
          var nextclass = (pos == nmotifs ? "navarrow inactive" : "navarrow");
          document.getElementById('prev_arrow_1').className = prevclass;
          document.getElementById('prev_arrow_2').className = prevclass;
          document.getElementById('next_arrow_1').className = nextclass;
          document.getElementById('next_arrow_2').className = nextclass;
          set_content_to_text(document.getElementById('pop_num_1'), pos);
          set_content_to_text(document.getElementById('pop_num_2'), pos);
        }
      }

      function both_change(inc) {
        var motif_num = parseInt(document.getElementById('submit_motif').value, 10);
        var nmotifs = parseInt(document.getElementById('nmotifs').value, 10);
        var orig = motif_num;
        motif_num += inc;
        if (motif_num > nmotifs) motif_num = nmotifs;
        else if (motif_num < 1) motif_num = 1;
        if (orig != motif_num) both_setup(motif_num);
      }

      function both_hide() {
        document.getElementById('grey_out_page').style.display = 'none';
        document.getElementById('download').style.display = 'none';
        document.getElementById('send_to').style.display = 'none';
      }

      /*
       * lookup the information on a motif and prepare the
       * popup for sending it to another program
       */
      function send_to_popup(pos) {
        both_setup(pos);
        var program = find_child_element_by_class(document.getElementById('programs'), 'selected').id;
        var task = highlight_submit_task(null, submit_programs[program]);
        highlight_submit_program(program, submit_tasks[task]);
        update_submit_text(task, program);
        // show the send to page
        var grey_out = document.getElementById('grey_out_page');
        grey_out.style.display = 'block';
        var send_to_pop = document.getElementById('send_to');
        send_to_pop.style.display = 'block';
      }

      function send_to_popup2(elem) {
        send_to_popup(expanded_num(elem));
      }

      function send_to_submit() {
        var program = find_child_element_by_class(document.getElementById('programs'), 'selected').id;
        // set the hidden fields on the form
        document.getElementById('submit_program').value = program;
        // send the form
        document.getElementById('submit_form').submit();
        both_hide();
      }

      function download_popup(pos) {
        both_setup(pos);
        click_download_tab(document.getElementById("download_tab_num").value);
        document.getElementById('submit_program').value = "LOGO";
        // show the download page
        var grey_out = document.getElementById('grey_out_page');
        grey_out.style.display = 'block';
        var download_pop = document.getElementById('download');
        download_pop.style.display = 'block';
      }

      function download_popup2(elem) {
        download_popup(expanded_num(elem));
      }

      function download_submit() {
        var format = document.getElementById('logo_format').value;
        var orient = document.getElementById('logo_rc').value;
        var ssc = document.getElementById('logo_ssc').value;
        var width = document.getElementById('logo_width').value;
        var height = document.getElementById('logo_height').value;
        document.getElementById('submit_format').value = format;
        document.getElementById('submit_rc').value = orient;
        document.getElementById('submit_ssc').value = ssc;
        document.getElementById('submit_width').value = width;
        document.getElementById('submit_height').value = height;
        document.getElementById('submit_form').submit();
        both_hide();
      }

      function FixLogoTask(num, rc) {
        this.num = num;
        this.rc = rc;
        this.run = FixLogoTask_run;
      }

      function FixLogoTask_run() {
        var pspm_text = document.getElementById("pspm" + this.num).value;
        var alpha = new Alphabet("ACGT");
        var pspm = new Pspm(pspm_text);
        if (this.rc) pspm = pspm.reverse_complement(alpha);
        var imgid = "small_" + (this.rc ? "rc_" : "") + "logo_" + this.num;

        var image = document.getElementById(imgid);

        var canvas = create_canvas(pspm.get_motif_length() *15, 50, image.id, 
            image.title, image.style.display);
        if (canvas == null) return;

        var logo = logo_1(alpha, "DREME", pspm);
        draw_logo_on_canvas(logo, canvas);
        image.parentNode.replaceChild(canvas, image);
      }

      function fix_popup_logo(image, canvasid, rc) {
        var motif_num = parseInt(document.getElementById('submit_motif').value, 10);
        var pspm_text = document.getElementById("pspm" + motif_num).value;
        var alpha = new Alphabet("ACGT");
        var pspm = new Pspm(pspm_text);
        if (rc) pspm = pspm.reverse_complement(alpha);
        image.style.display = "none";
        //check for canvas support before attempting anything
        var canvas = document.getElementById(canvasid);
        if (!canvas.getContext) return;
        if (!supports_text(canvas.getContext('2d'))) return;
        canvas.height = 90;
        canvas.width = 170;
        canvas.style.display = "inline";
        var logo = logo_1(alpha, "DREME", pspm);
        draw_logo_on_canvas(logo, canvas, false);
      }

      function fix_expanded_logo(image, rc) {
        var motif_num = expanded_num(image);
        var pspm_text = document.getElementById("pspm" + motif_num).value;
        var alpha = new Alphabet("ACGT");
        var pspm = new Pspm(pspm_text);
        if (rc) pspm = pspm.reverse_complement(alpha);
        //check for canvas support before attempting anything
        var canvas = document.createElement('canvas');
        if (!canvas.getContext) return;
        if (!supports_text(canvas.getContext('2d'))) return;
        canvas.height = 150;
        canvas.width = 0;
        draw_logo_on_canvas(logo_1(alpha, "DREME", pspm), canvas, false);
        image.parentNode.replaceChild(canvas, image);
      }

      function text_nodes(container) {
        var textNodes = [];
        var stack = [container];
        // depth first search to maintain ordering when flattened 
        while (stack.length > 0) {
          var node = stack.pop();
          if (node.nodeType == Node.TEXT_NODE) {
            textNodes.push(node);
          } else {
            for (var i = node.childNodes.length-1; i >= 0; i--) {
              stack.push(node.childNodes[i]);
            }
          }
        }
        return textNodes;
      }

      function node_text(node) {
        if (node === undefined) {
          return '';
        } else if (node.textContent) {
          return node.textContent;
        } else if (node.innerText) {
          return node.innerText;
        } else {
          return '';
        }
      }

      function nodes_text(nodes, separator) {
        if (separator === undefined) separator = '';
        var text = '';
        if (nodes.length > 0) {
          text += node_text(nodes[0]);
        }
        for (var i = 1; i < nodes.length; i++) {
          text += separator + node_text(nodes[i]);
        }
        return text;
      }

      function colour_dna_seq(container) {
        var textnodes = text_nodes(container);
        for (var i = 0; i < textnodes.length; i++) {
          var node = textnodes[i];
          container.replaceChild(create_dna_seq(node_text(node)), node);
        }
      }

      function create_dna_seq(seq) {
        var out = document.createElement('span');
        var last = 0;
        for (var i = 0; i < seq.length; i++) {
          var letter = seq.charAt(i);
          if (letter == 'A' || letter == 'C' || letter == 'G' || letter == 'T') { 
            if (last < i) {
              out.appendChild(document.createTextNode(seq.substring(last, i)));
            }
            var coloured_letter = document.createElement('span');
            coloured_letter.className = "dna_" + letter;
            coloured_letter.appendChild(document.createTextNode(letter));
            out.appendChild(coloured_letter);
            last = i + 1;
          }
        }
        if (last < seq.length) {
          out.appendChild(document.createTextNode(seq.substring(last)));
        }
        return out;
      }

      function sort_table(colEle, compare_function) {
        //find the parent of colEle that is either a td or th
        var cell = colEle;
        while (true) {
          if (cell == null) return;
          if (cell.nodeType == Node.ELEMENT_NODE && 
              (cell.tagName.toLowerCase() == "td" || cell.tagName.toLowerCase() == "th")) {
            break;
          }
          cell = cell.parentNode;
        }
        //find the parent of cell that is a tr
        var row = cell;
        while (true) {
          if (row == null) return;
          if (row.nodeType == Node.ELEMENT_NODE && row.tagName.toLowerCase() == "tr") {
            break;
          }
          row = row.parentNode;
        }
        //find the parent of row that is a table
        var table = row;
        while (true) {
          if (table == null) return;
          if (table.nodeType == Node.ELEMENT_NODE && table.tagName.toLowerCase() == "table") {
            break;
          }
          table = table.parentNode;
        }
        var column_index = cell.cellIndex;
        // do a bubble sort, because the tables are so small it doesn't matter
        var change;
        var trs = table.tBodies[0].getElementsByTagName('tr');
        var already_sorted = true;
        var reverse = false;
        while (true) {
          do {
            change = false;
            for (var i = 0; i < trs.length -1; i++) {
              var v1 = nodes_text(text_nodes(trs[i].cells[column_index]));
              var v2 = nodes_text(text_nodes(trs[i+1].cells[column_index]));
              if (reverse) {
                var tmp = v1;
                v1 = v2;
                v2 = tmp;
              }
              if (compare_function(v1, v2) > 0) {
                exchange(trs[i], trs[i+1], table);
                change = true;
                already_sorted = false;
                trs = table.tBodies[0].getElementsByTagName('tr');
              }
            }
          } while (change);
          if (reverse) break;// we've sorted twice so exit
          if (!already_sorted) break;// sort did something so exit
          // when it's sorted one way already then sort the opposite way
          reverse = true;
        }
        update_sort_arrows(row, column_index, reverse);
      }

      function update_sort_arrows(row, column_index, reverse) {
        var ascending = "\u25BC";
        var descending = "\u25B2";
        var dir = (reverse ? descending : ascending);
        for (var i = 0; i < row.cells.length; i++) {
          var arrow = find_child_element_by_class(row.cells[i], "sort_dir");
          if (arrow == null) continue;
          if (i == column_index) {
            set_content_to_text(arrow, dir);
          } else {
            set_content_to_text(arrow, "");
          }
        }
      }
    
      function exchange(oRowI, oRowJ, oTable) {
        var i = oRowI.rowIndex;
        var j = oRowJ.rowIndex;
         if (i == j+1) {
          oTable.tBodies[0].insertBefore(oRowI, oRowJ);
        } if (j == i+1) {
          oTable.tBodies[0].insertBefore(oRowJ, oRowI);
        } else {
            var tmpNode = oTable.tBodies[0].replaceChild(oRowI, oRowJ);
            if(typeof(oRowI) != "undefined") {
              oTable.tBodies[0].insertBefore(tmpNode, oRowI);
            } else {
              oTable.appendChild(tmpNode);
            }
        }
      }

      function compare_numbers(v1, v2) {
        var f1 = parseFloat(v1);
        var f2 = parseFloat(v2);
        if (f1 < f2) {
          return -1;
        } else if (f1 > f2) {
          return 1;
        } else {
          return 0;
        }
      }

      function compare_counts(v1, v2) {
        var re = /(\d+)\/\d+/;
        var m1 = re.exec(v1);
        var m2 = re.exec(v2);
        if (m1 == null && m2 == null) return 0;
        if (m1 == null) return -1;
        if (m2 == null) return 1;
        return compare_numbers(m1[1], m2[1]);
      }

      function compare_strings(v1, v2) {
        return v1.localeCompare(v2);
      }
      /*
       * coords
       *
       * Calculates the x and y offset of an element.
       * From http://www.quirksmode.org/js/findpos.html
       */
      function coords(elem) {
        var myX = myY = 0;
        if (elem.offsetParent) {
          do {
            myX += elem.offsetLeft;
            myY += elem.offsetTop;
          } while (elem = elem.offsetParent);
        }
        return [myX, myY];
      }
      /*
       * help
       *
       * Moves around help pop-ups so they appear
       * below an activator.
       */
      function help(activator, popup_id) {
        if (help.popup === undefined) {
          help.popup = null;
        }
        if (help.activator === undefined) {
          help.activator = null;
        }

        if (typeof(activator) == 'undefined') { // no activator so hide
          help.popup.style.display = 'none';
          help.popup = null;
          return;
        }
        var pop = document.getElementById(popup_id);
        if (pop == help.popup) {
          if (activator == help.activator) {
            //hide popup (as we've already shown it for the current help button)
            help.popup.style.display = 'none';
            help.popup = null;
            return; // toggling complete!
          }
        } else if (help.popup != null) {
          //activating different popup so hide current one
          help.popup.style.display = 'none';
        }
        help.popup = pop;
        help.activator = activator;

        //must make the popup visible to measure it or it has zero width
        pop.style.display = 'block';
        var xy = coords(activator);
        var padding = 10;
        var edge_padding = 15;
        var scroll_padding = 15;

        var pop_left = (xy[0] + (activator.offsetWidth / 2)  - (pop.offsetWidth / 2));
        var pop_top = (xy[1] + activator.offsetHeight + padding);

        // ensure the box is not past the top or left of the page
        if (pop_left < 0) pop_left = edge_padding;
        if (pop_top < 0) pop_top = edge_padding;
        // ensure the box does not cause horizontal scroll bars
        var page_width = null;
        if (window.innerWidth) {
          page_width = window.innerWidth;
        } else if (document.body) {
          page_width = document.body.clientWidth;
        }
        if (page_width) {
          if (pop_left + pop.offsetWidth > page_width) {
            pop_left = page_width - pop.offsetWidth - edge_padding - scroll_padding; //account for scrollbars
          }
        }

        pop.style.left = pop_left + "px";
        pop.style.top = pop_top + "px";
      }

      var submit_tasks = [];
      submit_tasks['search_motifs'] = ['TOMTOM'];
      submit_tasks['search_sequences'] = ['FIMO'];
      submit_tasks['rank_sequences'] = ['MAST'];
      submit_tasks['predict_go'] = ['GOMO'];
      submit_tasks['infer_tf'] = ['SPAMO'];
      var submit_programs = [];
      submit_programs['TOMTOM'] = ['search_motifs'];
      submit_programs['FIMO'] = ['search_sequences'];
      submit_programs['MAST'] = ['rank_sequences'];
      submit_programs['GOMO'] = ['predict_go'];
      submit_programs['SPAMO'] = ['infer_tf'];
      var submit_descriptions = [];
      submit_descriptions['TOMTOM'] = "Find similar motifs in published " + 
          "libraries or a library you supply.";
      submit_descriptions['FIMO'] = "Find motif occurences in sequence data.";
      submit_descriptions['MAST'] = "Rank sequences by affinity to groups " + 
          "of motifs.";
      submit_descriptions['GOMO'] = "Identify possible roles (Gene Ontology " + 
          "terms) for motifs.";
      submit_descriptions['SPAMO'] = "Find other motifs that are enriched at " +
          "specific close spacings which might imply the existance of a complex.";


      function click_submit_task(ele) {
        var task = ele.id;
        var program = highlight_submit_program(null, submit_tasks[task]);
        highlight_submit_task(task, submit_programs[program]);
        update_submit_text(task, program);
      }

      function click_submit_program(ele) {
        var program = ele.id;
        var task = highlight_submit_task(null, submit_programs[program]);
        highlight_submit_program(program, submit_tasks[task]);
        update_submit_text(task, program);
      }

      function update_submit_text(task, program) {
        var task_ele = document.getElementById(task);
        var program_ele = document.getElementById(program);
        set_content_to_text(document.getElementById('program_action'), 
            nodes_text(text_nodes(task_ele)));
        set_content_to_text(document.getElementById('program_name'), 
            nodes_text(text_nodes(program_ele)));
        set_content_to_text(document.getElementById('program_desc'), 
            submit_descriptions[program]);
      }

      function highlight_submit_task(select, highlights) {
        var tasks_ul = document.getElementById('tasks');
        var all_tasks = tasks_ul.getElementsByTagName('li');
        var li;
        var originally_selected = null;
        // deselect everything in the tasks list
        for (var i = 0; i < all_tasks.length; i++) {
          li = all_tasks[i];
          if (li.className == "selected") {
            originally_selected = li;
          }
          li.className = "";
        }
        // highlight everything in the highlights list
        for (var i = 0; i < highlights.length; i++) {
          var li = document.getElementById(highlights[i]);
          li.className = "active";
        }
        // check if we're setting the selected item specifically
        if (select != null) {
          li = document.getElementById(select);
          li.className = "selected";
          return select;
        } else {
          // if the originally selected item is allowed then keep it
          // otherwise move to the first element of the highlight list
          if (originally_selected != null && 
              originally_selected.className == "active") {
            originally_selected.className = "selected";
            return originally_selected.id;
          } else if (highlights.length > 0) {
            li = document.getElementById(highlights[0]);
            li.className = "selected";
            return highlights[0];
          }
          return null;
        }
      }


      function highlight_submit_program(select, highlights) {
        var programs_ul = document.getElementById('programs');
        var all_programs = programs_ul.getElementsByTagName('li');
        var li;
        var originally_selected = null;
        // deselect everything in the programs list
        for (var i = 0; i < all_programs.length; i++) {
          li = all_programs[i];
          if (li.className == "selected") {
            originally_selected = li;
          }
          li.className = "";
        }
        // highlight everything in the highlights list
        for (var i = 0; i < highlights.length; i++) {
          var li = document.getElementById(highlights[i]);
          li.className = "active";
        }
        // check if we're setting the selected item specifically
        if (select != null) {
          li = document.getElementById(select);
          li.className = "selected";
          return select;
        } else {
          // if the originally selected item is allowed then keep it
          // otherwise move to the first element of the highlight list
          if (originally_selected != null && 
              originally_selected.className == "active") {
            originally_selected.className = "selected";
            return originally_selected.id;
          } else if (highlights.length > 0) {
            li = document.getElementById(highlights[0]);
            li.className = "selected";
            return highlights[0];
          }
          return null;
        }
      }

      //
      // See http://stackoverflow.com/questions/814613/how-to-read-get-data-from-a-url-using-javascript
      // Slightly modified with information from
      // https://developer.mozilla.org/en/DOM/window.location
      //
      function parse_params() {
        var search = window.location.search;
        var queryStart = search.indexOf("?") + 1;
        var queryEnd   = search.indexOf("#") + 1 || search.length + 1;
        var query      = search.slice(queryStart, queryEnd - 1);
        var params  = {};

        if (query === search || query === "") return params;

        var nvPairs = query.replace(/\+/g, " ").split("&");

        for (var i=0; i<nvPairs.length; i++) {
          var nv = nvPairs[i].split("=");
          var n  = decodeURIComponent(nv[0]);
          var v  = decodeURIComponent(nv[1]);
          // allow a name to be used multiple times
          // storing each value in the array
          if ( !(n in params) ) {
            params[n] = [];
          }
          params[n].push(nv.length === 2 ? v : null);
        }
        return params;
      }

      /*
       * page_loaded
       *
       * Called when the page has loaded for the first time.
       */
      function page_loaded() {
        post_load_setup();
      }

      /*
       * page_loaded
       *
       * Called when a cached page is reshown.
       */
      function page_shown(e) {
        if (e.persisted) post_load_setup();
      }

      /*
       * post_load_setup
       *
       * Setup state that is dependant on everything having been loaded already.
       */
      function post_load_setup() {
        update_scrollpad();
        var params = parse_params();
        if (params["more"]) {
          var seq_to_num = {};
          for (var i = 0; i < motif_seqs.length; i++) {
            if (motif_seqs[i]) {
              seq_to_num[motif_seqs[i][1]] = i;
            }
          }
          var more_list = params["more"];
          for (var i = 0; i < more_list.length; i++) {
            var num = seq_to_num[more_list[i]];
            if (num !== undefined) {
              expand(num);
              var exp_row = document.getElementById('exp_row_' + num);
              exp_row.scrollIntoView(true);
            }
          }
          
        }
      }

      /*
       * update_scrollpad
       *
       * Creates padding at the bottom of the page to allow
       * scrolling of anything into view.
       */
      function update_scrollpad() {
        var elem = (document.compatMode === "CSS1Compat") ? 
            document.documentElement : document.body;
        document.getElementById("scrollpad").style.height = Math.abs(elem.clientHeight - 100) + "px";
      }

