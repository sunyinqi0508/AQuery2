import mariadb

class dbconn:
    def __init__(self) -> None:
        self.db = None
        self.cur = None
    def clear(self):
        drop_all = f'''
        SET FOREIGN_KEY_CHECKS = 0; 
        
        SET @tables = NULL;
        
        SELECT GROUP_CONCAT('`', table_schema, '`.`', table_name, '`') INTO @tables
        FROM information_schema.tables 
        WHERE table_schema = '{self.db.database}'; 

        SET @tables = CONCAT('DROP TABLE ', @tables);
        PREPARE stmt FROM @tables;
        EXECUTE stmt;
        DEALLOCATE PREPARE stmt;
        SET FOREIGN_KEY_CHECKS = 1; 
        '''
        if self.db:
            if not self.cur:
                self.cur = self.db.cursor()
            self.cur.execute(drop_all)
    
    def connect(self, ip, password = '0508', user = 'root', db = 'db', port = 3306):
        try:
            self.db = mariadb.connect(
                user = user,
                password = password,
                host = ip,
                port = port,
                database = db
            )
            self.cur = self.db.cursor()
            
        except mariadb.Error as e:
            print(e)
            self.db = None
            self.cur = None
            
    def exec(self, sql, params = None):
        self.cur.execute(sql)