//
// Created by Leonid on 25.09.2025.
//

#ifndef FILE_SYSTEM_COMMANDS_H
#define FILE_SYSTEM_COMMANDS_H

#define copy "cp";
#define copy_code 1;

#define move "mv";
#define move_code 2;


#define remove "rm";
#define remove_code 3;


#define make_directory "mkdir";
#define make_directory_code 4;


#define remove_directory "rmdir";
#define remove_directory_code 5;


#define list_contents "ls";
#define list_contents_code 6;


#define contains_at "cat";
#define contains_at_code 7;


#define change_directory "cd";
#define change_directory_code 8;

#define path "pwd";
#define path_code 9;


#define info "info"
#define info_code 10


#define move_into_filesystem "incp"
#define move_into_filesystem_code 11


#define move_out_of_filesystem "outcp"
#define move_out_of_filesystem_code 12


#define load_script "load"
#define load_script_code 13


#define format "format"
#define format_code 14


#define exit_manual "exit";
#define exit_manual_code 15;


#define statistics "statfs"
#define statistics_code 16

#define wrong_command_code 99


#endif //FILE_SYSTEM_COMMANDS_H