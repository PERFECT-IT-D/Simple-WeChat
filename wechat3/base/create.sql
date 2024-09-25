CREATE TABLE `t_wechat_users`
(
    `user_id`           int         NOT NULL AUTO_INCREMENT,
    `user_name`         varchar(50) NOT NULL,
    `user_passwd`       varchar(50) NOT NULL,
    `RegistrationDate` date     DEFAULT NULL,
    `LastLogin`        datetime DEFAULT NULL,
    PRIMARY KEY (`user_id`),
    INDEX              `idx_user_id_name` (`user_id`, `user_name`)
);
CREATE TABLE `t_friend_ship`
(
    `id`        INT NOT NULL AUTO_INCREMENT,
    `user_id`   INT NOT NULL,
    `friend_id` INT NOT NULL,
    PRIMARY KEY (`id`),
    UNIQUE INDEX `idx_unique_friendship` (`user_id`, `friend_id`),
    FOREIGN KEY (`user_id`) REFERENCES `t_wechat_users` (`user_id`),
    FOREIGN KEY (`friend_id`) REFERENCES `t_wechat_users` (`user_id`)
)
CREATE TABLE `t_save_msg`
(
    `msg_id`    INT NOT NULL AUTO_INCREMENT,
    `user_id`   INT NOT NULL,
    `msg`       VARCHAR(1024),
    `friend_id` INT NOT NULL,
    PRIMARY KEY (`msg_id`),
    INDEX       `idx_friend_id` (`friend_id`),
)