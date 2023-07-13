CREATE TABLE xfers (
    xferID 	int unsigned auto_increment primary key not null,
    Data	datetime,
    Time	int,
    User	VARCHAR(50) NOT NULL,
    Host	VARCHAR(50) NOT NULL,
    Upload	VARCHAR(1),
    Size	int(10) unsigned,
    File	VARCHAR(255)
);