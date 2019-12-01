#include "database.hpp"

// create parse
 void Database::create_parse(const vector<string>& query, string& table_name, map<string, string>& fields_result){
    int i = 0;
    string qs;
    while(i < query.size()){
        qs += query[i];
        i++;
    }
    // find the left and right bracket after "create table tablename(...)"
    int bracket_left = qs.find_first_of("(");
    if(bracket_left < 0){
        Error("syntax error around tablenam");
    }
    int bracket_right = qs.find_last_of(")");
    if(bracket_right < 0){
        Error("syntax error around tablename");
    }
    /* First substr the tablename */
    table_name = qs.substr(0, bracket_left);
    qs = qs.substr(bracket_left, bracket_right);

    /* Second parse the fields */
    vector<string> fields;
    // split the sql with comma-","
    int comma_pos = comma_pos = qs.find_first_of(",");
    while(comma_pos >= 0){
        fields.push_back(qs.substr(0, comma_pos));
        qs = qs.substr(comma_pos+1, qs.size());
        comma_pos = qs.find_first_of(",");
    }
    if(fields.size < 1){
        Error("There is not any field!");
    }
    // trim " filed type " to "field type"
    for(i = 0;i<fields.size();i++){
        trim(fields[i]);
    }
    // field_result to store the result of parsing fields
    int j;
    vector<string> temp;
    for(i = 0;i<fields.size();i++){
        // 1.find the "()" of "name cahr(10)" or "primary key()"
        bracket_left = fields[i].find_first_of("(");
        bracket_right = fields[i].find_last_of(")");
        if(bracket_left * bracket_right < 0){       // there are only one side can match
            Error("brackets match error");
        }

        split(fields[i], ' ', temp);
        // there is not any brackets, so just like "id int"
        if(bracket_left < 0){   
            temp.clear();
            trim(temp[1]);
            fields_result[temp[0]] = temp[1] + " " + to_string(4);
            continue;
        }
        // there are some brackets
        // three conditions
        // 1. filed type contains () like char varcahr enum 
        int brack_pos_l = temp[1].find_first_of("(");
        int brack_pos_r = temp[1].find_last_of(")");
        // (var)char
        int pos = fields[i].find("char");
        if(pos != fields[i].npos){
            if(!is_digit_str(temp[1].substr(brack_pos_l+1, brack_pos_r-1))){
                Error("char length error");
            }
            if(pos > 2 && fields[i].substr(pos-3,pos) == "var"){
                fields_result[temp[0]] = "varchar " + temp[1].substr(brack_pos_l+1, brack_pos_r-1);
            }else{
                fields_result[temp[0]] = "char " + temp[1].substr(brack_pos_l+1, brack_pos_r-1);
            }
            continue;
        }
        // enum
        pos = fields[i].find("enum");
        if(pos != fields[i].npos){
            string t = fields[i].substr(brack_pos_l+1, brack_pos_r-1);
            vector<string> ts;
            split(t, ',', ts);
            string type_all;
            int max_len_of_enum = 0;
            j = 0;
            while(j < ts.size()){
                trim(ts[j]);
                ts[j] = ts[j].substr(1, ts[j].size()-1);    // discard the ""
                if(ts[j].size() > max_len_of_enum){
                    max_len_of_enum = ts[j].size();
                }
                type_all += (" " + ts[j]);
                j++;
            } 
            fields_result[temp[0]] = "enum " + to_string(max_len_of_enum) + " " + to_string(ts.size()) + type_all;
            continue;
        }
        // primary
        pos = fields[i].find("primary");
        if(pos != fields[i].npos){
            trim(fields[1]);
            int pos_small_l = fields[1].find_first_of("(");
            int pos_small_r = fields[1].find_first_of(")");
            string key_name = fields[1].substr(0, pos_small_l-1);
            string field_name =  fields[1].substr(pos_small_l+1, pos_small_r-1);
            if(fields_result.count(field_name) < 1){
                Error("primary key error");
            }
            fields_result[key_name] = "primary " + field_name;
            continue;
        }
        // index
        pos = fields[i].find("index");
        if(pos != fields[i].npos){
            trim(fields[1]);
            int pos_small_l = fields[1].find_first_of("(");
            int pos_small_r = fields[1].find_first_of(")");
            string key_name = fields[1].substr(0, pos_small_l-1);
            string field_name =  fields[1].substr(pos_small_l+1, pos_small_r-1);
            vector<string> ts;
            split(field_name, ',', ts);
            field_name = "";
            j = 0;
            while(j < ts.size()){
                trim(ts[j]);
                if(fields_result.count(ts[j]) < 1){
                    Error("index key error");
                }
                field_name += ( " " + ts[j] );
            }
            fields_result[key_name] = "index" + field_name;
            continue;
        }
    }
}
   
// 以下几个操作注意约束和限制条件(留坑完事)
// 插入
void Database::Insert(){
    
}

// create table
void Database::Create(const vector<string> query)
{
    /* create table information file */
    // srtucture
    // db file name
    // fields
    // keys
    map<string, string> fields_result;
    string table_name;
    create_parse(query, table_name, fields_result);
    CreateFile(table_name, fields_result);
    LoadTable(table_name);
}

// 删表
void Database::Drop()
{}

// delete query
void Database::Delete()
{}

// update query
void Database::Update()
{}

// selectc query
void Database::Select()
{}





void Database::CreateFile(string& table_name, map<string, string> fields_result) {
    // First. write the tablename to table_name.txt(whch contains all table names)
	fstream fs("table_name.txt");
	if(table_exists(table_name)){
        Error("table exists");
    }
	if (table_name.size() > 20)
	{
		Error("Table name length too long!");
	}
	fs << table_name << endl;
    fs.clear();


    // Second. write the table_meta to "table_name".txt

	table_meta_t table_meta;
    key_meta_header priKey_meta;
    data_meta_header data_meta;
    bool have_priKey = false;
    // table_mate
    map<string, string>::iterator iter = fields_result.begin();
    stringstream ss;
    string field_name;
    string field_type_s;
    string pri_field_name;
    int field_type;
    int field_len;
    while (iter != fields_result.end())
    {
        ss.clear();
        ss<<iter->second;
        field_name = iter->first;
        ss>>field_type_s;
        field_type = parse_type(field_type_s);
        ss>>field_len;

        if(field_type == 7){
            Error("FIELD_TYPE Error");
            exit(0);
        }else if(field_type == 5){  // primaey key
            have_priKey = true;
            memcpy(table_meta.pri_name, field_name.c_str(), field_name.size());
            ss>>pri_field_name;
            memcpy(table_meta.pri_field_name, pri_field_name.c_str(), 
                                            pri_field_name.size());
        }else if(field_type == 6){ // index key
            memcpy(table_meta.indexs[table_meta.index_count], 
                            field_name.c_str(), field_name.size());
            field_len = 0;
            int j=0;
            while(ss>>field_name){
                for(;j<table_meta.fields_count;j++){
                    if(strcmp((const char *)table_meta.fields_name[j], field_name.c_str()) == 0){
                        field_len += table_meta.fields_len[j];
                    }
                }
                j = 0;
            }
            table_meta.indexs_max_len[table_meta.index_count] = field_len;
            table_meta.index_count += 1;
        }else{  // fields
            table_meta.fields_type[table_meta.fields_count] = field_type;
            memcpy(table_meta.fields_name[table_meta.fields_count], 
                            field_name.c_str(), field_name.size());
            table_meta.fields_len[table_meta.fields_count] = field_len;
            table_meta.fields_count += 1;
        }
    }
    // set the max_len of primary key
    typedef int key_type;
    int i = 0;
    while(i < table_meta.fields_count){
        if(strcmp(table_meta.pri_field_name, table_meta.fields_name[i]) == 0){
                table_meta.pri_max_len = table_meta.fields_len[i];
                string tp = get_type_str(table_meta.fields_type[i]);
                if(tp == "int"){
                    typedef int key_type;
                }
                break;
        }
        i++;
    }
    fs.open(table_name+".txt");
    fs.write((const char *)&table_meta, sizeof(table_meta));
    fs.clear();

    if(!access("./" + table_name)){
        mkdir("./" + table_name);
    }
    // Third. Set the key file header
    key_meta_t key_meta;
    // Set the primary key file meta_header
    bzero(&key_meta, sizeof(key_meta_header));  // 与 memset的区别就是bzero直接设置为\0 memset可以设替换的ch
    key_meta.key_size = table_meta.pri_max_len;
    key_meta.height = 1;
    key_meta.internal_node_num = 1;
    key_meta.leaf_node_num = 1;
    // slot theposition which block begin 
    key_meta.slot = OFFSET_BLOCK;
    
    key_meta.page_count = 4;
    key_meta.un_count = 0;
    key_meta.max_size = 0;
    key_meta.unsorted = 0;
    key_meta.max_unsorted = 0;

    // init root node
    internal_node_t<key_type> root;
    root.next = root.prev = root.parent = 0;
    key_meta.root_offset = key_meta.slot;
    key_meta.slot += sizeof(root);

    // init empty leaf
    internal_node_t<key_type> leaf;
    leaf.next = leaf.prev = 0;
    leaf.parent = key_meta.root_offset;
    key_meta.leaf_offset = root.children[0].child = key_meta.slot;
    key_meta.slot += sizeof(leaf);

    fs.open(table_name + ".key");
    fs.write((const char *)&key_meta, sizeof(key_meta));
    fs.clear();

    // Set the indedx key file meta_header
    if(table_meta.index_count > 0){
        i = 0; 
        for(;i<table_meta.index_count;i++){
            key_meta.key_size = table_meta.indexs_max_len[i];
            fs.open("./" + table_name + "/" + string(table_meta.indexs[i])+".idx", ios::out);
            fs.write((const char *)&key_meta, sizeof(key_meta));
            fs.clear();
        }
    }


    // Forth. Set the data(.db) file header
     // create .db(data file) .key(primaey index file) .idx(index key file)
	fs.open(table_name + ".db", ios::out);     // create file automatic
    data_meta_header data_meta;
    data_meta.begin_offset = 4096;
    data_meta.data_count = 0;
    data_meta.max_size = 0;
    data_meta.max_unsorted = 0;
    data_meta.slot = 4096;
    data_meta.un_count = 0;
    data_meta.unsorted = 0;

    fs.write((const char *)&data_meta, sizeof(data_meta));
    fs.close();
}

/*
    Load the table_meta information and primary key file meta()
*/
void LoadTable(string& table_name){
    // load table_meta 
    while(table_opened > MAX_OPEN_TABLE){
        sleep(0.1);
    }
    table_count_mutex.clock();
    table_opened += 1;      // provent conflict
    table_count_mutex.unlock();
    int fd = open(table_name+".txt", O_RDWR);
    char * buf = mmap(NULL, sizeof(table_meta_t), 
                                    PROT_READ|PROT_WRITE, MAP_SHAREd, fd, 0);
    if(buf == MAP_FAILED){
        Error("table meta information mmap failed");
    }
    tableFd[table_opened - 1] = fd;
    table_name_idx[table_name] = table_opened - 1;
    tableMeta[table_opened - 1] = buf;

    // set table opened bit map
    int pos = get_next_table_pos();
    set_next_table_pos(pos);
    
    // load primary key file
    table_meta_header * k_p = (table_meta_header)buf;
    fd = open(table_name + ".key", O_RDWR);
    tab_key_fd[table_name][k_p->pri_name] = fd;
    write(fd, (const void *)tablePriMeta[table_opened - 1], szieof(key_meta_header)));
}

/*
    @params table_name  :
            key_name    :   the key which used to search(complex search)
*/
void LoadTableIndex(string& table_name, string& key_name){
    string file_path = "./" + table_name + "/" + key_name;
    int fd = open(file_path, O_RDWR);
    if(fd != -1){
        tab_key_fd[table_name][key_name] =fd;
        int pos = table_name_idx[table_name]; 
        write(fd, (const void *)indexs_meta[pos].second, szieof(key_meta_header)));
    }else{
        Error("Error while opening key file");
    }
}

/*
    @params table_name
            memCacheId : which memCache the new page should be put.
            posId      : which position in memCache(upper) should be put.
            pageId     : which page should be load in.
    @return bool if success
*/
bool Database::LoadPage(string & table_name, int memCacheId, int posId, int pageId){
    int fd = open(table_name+".db", O_RDWR);
    tableData[table_name_idx[table_name]] = fd;
    char * buf = mmap(memCache[memCacheId]+4096*posId, 4096, 
                        PROT_READ|PROT_WRITE, MAP_SHARED, fd, (pageId+1)*4096);
    if(buf == MAP_FAILED){
        Error("Load Page data Failed!");
    }
}

bool table_exists(string table_name){
    fstream fs("table_name.txt", ios::out);
    char buffer[20];
	while (!fs.eof()) {
		fs.getline(buffer, 20);
		if (strcmp(buffer, table_name.c_str()) == 0) {
			return true;
		}
	}
    return false;
}