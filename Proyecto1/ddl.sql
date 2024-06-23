
CREATE DATABASE so2p1;
USE so2p1;

create table memory_requests(
    id bigint primary key auto_increment,
    pid bigint,
    process_name text,
    is_mmap bool,
    unix_time bigint
);
